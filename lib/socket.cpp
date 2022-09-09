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

static unsigned short Addrinfo_afamily(Env *env, Value family) {
    if (family->is_string() || family->is_symbol()) {
        auto sym = family->to_symbol(env, Object::Conversion::Strict);
        if (sym == "INET"_s)
            sym = "AF_INET"_s;
        else if (sym == "INET6"_s)
            sym = "AF_INET6"_s;
        return find_top_level_const(env, "Socket"_s)->const_find(env, sym)->as_integer()->to_nat_int_t();
    } else if (family->is_integer()) {
        return family->as_integer()->to_nat_int_t();
    }
    return 0;
}

Value Addrinfo_initialize(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 1, 4);
    auto sockaddr = args.at(0);
    auto afamily = Addrinfo_afamily(env, args.at(1, NilObject::the()));
    auto socktype = args.at(2, NilObject::the());
    auto protocol = args.at(3, Value::integer(0));

    self->ivar_set(env, "@protocol"_s, protocol);

    if (socktype->is_string() || socktype->is_symbol()) {
        auto sym = socktype->to_symbol(env, Object::Conversion::Strict);
        if (sym == "STREAM"_s)
            sym = "SOCK_STREAM"_s;
        socktype = find_top_level_const(env, "Socket"_s)->const_find(env, sym);
    }

    StringObject *host = nullptr;
    IntegerObject *port = nullptr;
    StringObject *unix_path = nullptr;

    if (!afamily)
        self->ivar_set(env, "@pfamily"_s, Value::integer(PF_UNSPEC));

    if (sockaddr->is_string() && sockaddr->as_string()->length() > 0) {
        int first_byte = sockaddr->as_string()->string().at(0);
        switch (first_byte) {
        case AF_UNIX:
            afamily = first_byte;
            unix_path = GlobalEnv::the()->Object()->const_fetch("Socket"_s).send(env, "unpack_sockaddr_un"_s, { sockaddr })->as_string();
            break;
        case AF_INET:
        case AF_INET6:
            afamily = first_byte;
            auto ary = GlobalEnv::the()->Object()->const_fetch("Socket"_s).send(env, "unpack_sockaddr_in"_s, { sockaddr })->as_array();
            port = ary->at(0)->as_integer();
            host = ary->at(1)->as_string();
            break;
        }
    }

    if (sockaddr->is_array()) {
        // initialized with array like ["AF_INET", 49429, "hal", "192.168.0.128"]
        //                          or ["AF_UNIX", "/tmp/sock"]
        auto ary = sockaddr->as_array();
        afamily = Addrinfo_afamily(env, ary->at(0));
        self->ivar_set(env, "@afamily"_s, Value::integer(afamily));
        self->ivar_set(env, "@pfamily"_s, Value::integer(afamily));
        switch (afamily) {
        case AF_UNIX:
            unix_path = ary->at(1)->as_string();
            break;
        case AF_INET:
        case AF_INET6:
            port = ary->at(1)->as_integer();
            host = ary->at(2)->as_string();
            if (ary->at(3)->is_string())
                host = ary->at(3)->as_string();
            break;
        default:
            env->raise("ArgumentError", "bad sockaddr");
        }
    }

    if (afamily == AF_UNIX) {
        assert(unix_path);
        self->ivar_set(env, "@afamily"_s, Value::integer(AF_UNIX));
        self->ivar_set(env, "@unix_path"_s, unix_path);

    } else {
        assert(host);
        assert(port);

        struct addrinfo hints { };
        struct addrinfo *getaddrinfo_result;

        if (afamily)
            hints.ai_family = afamily;

        if (socktype->is_integer())
            hints.ai_socktype = (unsigned short)socktype->as_integer()->to_nat_int_t();
        else
            self->ivar_set(env, "@socktype"_s, Value::integer(0));

        if (protocol->is_integer())
            hints.ai_protocol = (unsigned short)protocol->as_integer()->to_nat_int_t();

        const char *service_str = port->to_s(env)->as_string()->c_str();

        if (hints.ai_socktype == SOCK_RAW)
            service_str = nullptr;

        int s = getaddrinfo(
            host->c_str(),
            service_str,
            &hints,
            &getaddrinfo_result);
        // printf("socktype=%d, protocol=%d, host=%s, port=%s, family=%d\n",
        // hints.ai_socktype,
        // hints.ai_protocol,
        // host->c_str(),
        // port->inspect_str(env)->c_str(),
        // hints.ai_family);
        if (s != 0)
            env->raise("SocketError", "getaddrinfo: {}", gai_strerror(s));
        if (self->ivar_get(env, "@pfamily"_s)->is_nil())
            self->ivar_set(env, "@pfamily"_s, Value::integer(getaddrinfo_result->ai_family));
        if (self->ivar_get(env, "@socktype"_s)->is_nil())
            self->ivar_set(env, "@socktype"_s, Value::integer(getaddrinfo_result->ai_socktype));

        if (getaddrinfo_result->ai_family != afamily)
            env->raise("SocketError", "getaddrinfo: Address family for hostname not supported");

        switch (getaddrinfo_result->ai_family) {
        case AF_INET: {
            self->ivar_set(env, "@afamily"_s, Value::integer(AF_INET));
            char address[INET_ADDRSTRLEN];
            auto *sockaddr = (struct sockaddr_in *)getaddrinfo_result->ai_addr;
            inet_ntop(AF_INET, &(sockaddr->sin_addr), address, INET_ADDRSTRLEN);
            self->ivar_set(env, "@ip_address"_s, new StringObject { address });
            auto port_in_network_byte_order = sockaddr->sin_port;
            self->ivar_set(env, "@ip_port"_s, Value::integer(ntohs(port_in_network_byte_order)));
            break;
        }
        case AF_INET6: {
            self->ivar_set(env, "@afamily"_s, Value::integer(AF_INET6));
            char address[INET6_ADDRSTRLEN];
            auto *sockaddr = (struct sockaddr_in6 *)getaddrinfo_result->ai_addr;
            inet_ntop(AF_INET6, &(sockaddr->sin6_addr), address, INET6_ADDRSTRLEN);
            self->ivar_set(env, "@ip_address"_s, new StringObject { address });
            auto port_in_network_byte_order = sockaddr->sin6_port;
            self->ivar_set(env, "@ip_port"_s, Value::integer(ntohs(port_in_network_byte_order)));
            break;
        }
        default:
            env->raise("SocketError", "getaddrinfo: unrecognized result");
        }

        freeaddrinfo(getaddrinfo_result);
    }

    return self;
}

Value Socket_pack_sockaddr_in(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 0, 2);
    auto service = args.at(0, NilObject::the());
    auto host = args.at(1, NilObject::the());
    if (host->is_nil())
        host = new StringObject { "127.0.0.1" };
    if (host->is_string() && host->as_string()->is_empty())
        host = new StringObject { "0.0.0.0" };
    host->assert_type(env, Object::Type::String, "String");
    auto host_string = host->as_string();

    unsigned short port_in_host_byte_order = 0;
    if (service->is_integer()) {
        auto num = service->as_integer()->to_nat_int_t();
        if (num >= 0 && num <= 65535)
            port_in_host_byte_order = num;
    } else if (service->is_string() && service->as_string()->string().contains_only_digits()) {
        port_in_host_byte_order = service->as_string()->to_i(env)->as_integer()->to_nat_int_t();
    } else if (service->is_string()) {
        auto servent = getservbyname(service->as_string()->c_str(), nullptr);
        if (servent)
            port_in_host_byte_order = ntohs(servent->s_port);
    }

    if (host_string->include(":")) {

        // IPV6
        struct sockaddr_in6 in { };
#if defined(__OpenBSD__) or defined(__APPLE__)
        in.sin6_len = sizeof(in);
#endif
        in.sin6_family = AF_INET6;
        in.sin6_port = htons(port_in_host_byte_order);
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
        in.sin_port = htons(port_in_host_byte_order);
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

    if (sockaddr->is_a(env, self->const_find(env, "Addrinfo"_s, Object::ConstLookupSearchMode::NotStrict))) {
        auto afamily = sockaddr.send(env, "afamily"_s).send(env, "to_i"_s)->as_integer()->to_nat_int_t();
        if (afamily != AF_INET && afamily != AF_INET6)
            env->raise("ArgumentError", "not an AF_INET/AF_INET6 sockaddr");
        auto host = sockaddr.send(env, "ip_address"_s);
        auto port = sockaddr.send(env, "ip_port"_s);
        return new ArrayObject { port, host };
    }

    sockaddr->assert_type(env, Object::Type::String, "String");

    if (sockaddr->as_string()->length() == sizeof(struct sockaddr_in6)) {

        // IPV6
        const char *str = sockaddr->as_string()->c_str();
        auto in = (struct sockaddr_in6 *)str;
        auto ary = new ArrayObject;
        auto port_in_network_byte_order = in->sin6_port;
        ary->push(Value::integer(ntohs(port_in_network_byte_order)));
        ary->push(new StringObject((const char *)in->sin6_addr.s6_addr));
        return ary;

    } else if (sockaddr->as_string()->length() == sizeof(struct sockaddr_in)) {

        // IPV4
        const char *str = sockaddr->as_string()->c_str();
        auto in = (struct sockaddr_in *)str;
        auto addr = inet_ntoa(in->sin_addr);
        auto ary = new ArrayObject;
        auto port_in_network_byte_order = in->sin_port;
        ary->push(Value::integer(ntohs(port_in_network_byte_order)));
        ary->push(new StringObject(addr));
        return ary;

    } else {
        env->raise("ArgumentError", "not an AF_INET/AF_INET6 sockaddr");
    }
}

Value Socket_unpack_sockaddr_un(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 0, 1);
    auto sockaddr = args.at(0, NilObject::the());

    if (sockaddr->is_a(env, self->const_find(env, "Addrinfo"_s, Object::ConstLookupSearchMode::NotStrict))) {
        auto afamily = sockaddr.send(env, "afamily"_s).send(env, "to_i"_s)->as_integer()->to_nat_int_t();
        if (afamily != AF_UNIX)
            env->raise("ArgumentError", "not an AF_UNIX sockaddr");
        return sockaddr.send(env, "unix_path"_s);
    }

    sockaddr->assert_type(env, Object::Type::String, "String");

    if (sockaddr->as_string()->length() != sizeof(struct sockaddr_un))
        env->raise("ArgumentError", "not an AF_UNIX sockaddr");

    const char *str = sockaddr->as_string()->c_str();
    auto un = (struct sockaddr_un *)str;
    return new StringObject { un->sun_path };
}
