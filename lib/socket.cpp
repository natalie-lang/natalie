#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "natalie.hpp"

using namespace Natalie;

Value init(Env *env, Value self) {
    return NilObject::the();
}

Value Addrinfo_initialize(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 1, 4);
    auto sockaddr = args.at(0);
    auto family = args.at(1, NilObject::the());
    auto socktype = args.at(2, NilObject::the());
    auto protocol = args.at(3, Value::integer(0));

    self->ivar_set(env, "@sockaddr"_s, sockaddr);
    self->ivar_set(env, "@protocol"_s, protocol);

    if (family->is_string() || family->is_symbol()) {
        auto sym = family->to_symbol(env, Object::Conversion::Strict);
        if (sym == "INET"_s)
            sym = "PF_INET"_s;
        family = find_top_level_const(env, "Socket"_s)->const_find(env, sym);
    }

    if (socktype->is_string() || socktype->is_symbol()) {
        auto sym = socktype->to_symbol(env, Object::Conversion::Strict);
        if (sym == "STREAM"_s)
            sym = "SOCK_STREAM"_s;
        socktype = find_top_level_const(env, "Socket"_s)->const_find(env, sym);
    }

    struct addrinfo hints { };
    struct addrinfo *result {};

    if (family->is_integer())
        hints.ai_family = (unsigned short)family->as_integer()->to_nat_int_t();
    else
        self->ivar_set(env, "@pfamily"_s, Value::integer(PF_UNSPEC));

    if (socktype->is_integer())
        hints.ai_socktype = (unsigned short)socktype->as_integer()->to_nat_int_t();

    hints.ai_flags = 0;

    struct sockaddr_un un { };
    if (sockaddr->as_string()->bytesize() == sizeof(un)) {
        auto path = GlobalEnv::the()->Object()->const_fetch("Socket"_s).send(env, "unpack_sockaddr_un"_s, { sockaddr });
        self->ivar_set(env, "@afamily"_s, Value::integer(AF_UNIX));
        self->ivar_set(env, "@unix_path"_s, path);
    } else {
        auto ary = GlobalEnv::the()->Object()->const_fetch("Socket"_s).send(env, "unpack_sockaddr_in"_s, { sockaddr })->as_array();
        auto port = ary->at(0);
        auto host = ary->at(1);
        int s = getaddrinfo(
            host->as_string()->c_str(),
            port->as_integer()->to_s(env)->as_string()->c_str(),
            &hints,
            &result);
        if (s != 0)
            env->raise("SocketError", "getaddrinfo: {}\\n", gai_strerror(s));
        self->ivar_set(env, "@afamily"_s, Value::integer(AF_INET));
        if (self->ivar_get(env, "@pfamily"_s)->is_nil())
            self->ivar_set(env, "@pfamily"_s, Value::integer(result->ai_family));
        self->ivar_set(env, "@socktype"_s, Value::integer(result->ai_socktype));
        switch (result->ai_family) {
        case AF_INET: {
            char address[INET_ADDRSTRLEN];
            auto *sockaddr = (struct sockaddr_in *)result->ai_addr;
            inet_ntop(AF_INET, &(sockaddr->sin_addr), address, INET_ADDRSTRLEN);
            self->ivar_set(env, "@ip_address"_s, new StringObject { address });
            break;
        }
        case AF_INET6: {
            char address[INET6_ADDRSTRLEN];
            auto *sockaddr = (struct sockaddr_in6 *)result->ai_addr;
            inet_ntop(AF_INET6, &(sockaddr->sin6_addr), address, INET6_ADDRSTRLEN);
            self->ivar_set(env, "@ip_address"_s, new StringObject { address });
            break;
        }
        default:
            NAT_UNREACHABLE();
        }
    }

    return self;
}

Value Socket_pack_sockaddr_in(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 0, 2);
    auto port = args.at(0, NilObject::the());
    auto host = args.at(1, NilObject::the());
    if (host->is_nil())
        host = new StringObject { "127.0.0.1" };
    if (host->is_string() && host->as_string()->is_empty())
        host = new StringObject { "0.0.0.0" };
    host->assert_type(env, Object::Type::String, "String");
    auto host_string = host->as_string();

    if (!port->is_integer() && port->respond_to(env, "to_i"_s))
        port = port->send(env, "to_i"_s);

    port->assert_type(env, Object::Type::Integer, "Integer");
    auto port_unsigned = (unsigned short)port->as_integer()->to_nat_int_t();

    if (host_string->include(":")) {

        // IPV6
        struct sockaddr_in6 in { };
#if defined(__OpenBSD__) or defined(__APPLE__)
        in.sin6_len = sizeof(in);
#endif
        in.sin6_family = AF_INET6;
        in.sin6_port = port_unsigned;
        memcpy(in.sin6_addr.s6_addr, host_string->c_str(), std::min(host_string->length(), (size_t)16));
        return new StringObject { (const char *)&in, sizeof(in) };

    } else {

        // IPV4
        struct in_addr a;
        auto result = inet_aton(host->as_string()->c_str(), &a);
        if (!result)
            env->raise("SocketError", "getaddrinfo: Name or service not known");
        struct sockaddr_in in { };
#if defined(__OpenBSD__) or defined(__APPLE__)
        in.sin_len = sizeof(in);
#endif
        in.sin_family = AF_INET;
        in.sin_port = port_unsigned;
        in.sin_addr = a;
        return new StringObject { (const char *)&in, sizeof(in) };
    }
}

Value Socket_pack_sockaddr_un(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 0, 1);
    auto path = args.at(0, NilObject::the());
    path->assert_type(env, Object::Type::String, "String");
    auto path_string = path->as_string();

    struct sockaddr_un un { };
    constexpr auto unix_path_max = sizeof(un.sun_path);

    if (path_string->length() >= unix_path_max)
        env->raise("ArgumentError", "too long unix socket path ({} bytes given but {} bytes max))", path_string->length(), unix_path_max);

    un.sun_family = AF_UNIX;
    memcpy(un.sun_path, path_string->c_str(), path_string->length());
    return new StringObject { (const char *)&un, sizeof(un) };
}

Value Socket_unpack_sockaddr_in(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 0, 1);
    auto sockaddr = args.at(0, NilObject::the());
    if (sockaddr->is_a(env, self->const_find(env, "Addrinfo"_s, Object::ConstLookupSearchMode::NotStrict)))
        sockaddr = sockaddr->send(env, "sockaddr"_s);

    sockaddr->assert_type(env, Object::Type::String, "String");

    if (sockaddr->as_string()->length() == sizeof(struct sockaddr_in6)) {

        // IPV6
        const char *str = sockaddr->as_string()->c_str();
        auto in = (struct sockaddr_in6 *)str;
        auto ary = new ArrayObject;
        ary->push(Value::integer(in->sin6_port));
        ary->push(new StringObject((const char *)in->sin6_addr.s6_addr));
        return ary;

    } else if (sockaddr->as_string()->length() == sizeof(struct sockaddr_in)) {

        // IPV4
        const char *str = sockaddr->as_string()->c_str();
        auto in = (struct sockaddr_in *)str;
        auto addr = inet_ntoa(in->sin_addr);
        auto ary = new ArrayObject;
        ary->push(Value::integer(in->sin_port));
        ary->push(new StringObject(addr));
        return ary;

    } else {
        env->raise("ArgumentError", "not an AF_INET/AF_INET6 sockaddr");
    }
}

Value Socket_unpack_sockaddr_un(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 0, 1);
    auto sockaddr = args.at(0, NilObject::the());
    if (sockaddr->is_a(env, self->const_find(env, "Addrinfo"_s, Object::ConstLookupSearchMode::NotStrict)))
        sockaddr = sockaddr->send(env, "sockaddr"_s);

    sockaddr->assert_type(env, Object::Type::String, "String");

    if (sockaddr->as_string()->length() != sizeof(struct sockaddr_un))
        env->raise("ArgumentError", "not an AF_UNIX sockaddr");

    const char *str = sockaddr->as_string()->c_str();
    auto un = (struct sockaddr_un *)str;
    return new StringObject { un->sun_path };
}
