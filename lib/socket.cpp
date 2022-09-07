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
    if (!family->is_nil())
        hints.ai_family = (unsigned short)family->as_integer()->to_nat_int_t();
    if (!socktype->is_nil())
        hints.ai_socktype = (unsigned short)socktype->as_integer()->to_nat_int_t();
    hints.ai_flags = 0;
    // hints.ai_protocol = IPPROTO_UDP;

    struct sockaddr_un un { };
    if (sockaddr->as_string()->bytesize() == sizeof(un)) {
        auto path = GlobalEnv::the()->Object()->const_fetch("Socket"_s).send(env, "unpack_sockaddr_un"_s, { sockaddr });
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
        self->ivar_set(env, "@_addrinfo"_s, new VoidPObject { result });
    }

    return self;
}

Value Addrinfo_ip_address(Env *env, Value self, Args args, Block *block) {
    auto addrinfo = self->ivar_get(env, "@_addrinfo"_s);
    if (addrinfo->is_nil())
        env->raise("SocketError", "need IPv4 or IPv6 address");

    auto info_struct = (struct addrinfo *)addrinfo->as_void_p()->void_ptr();

    switch (info_struct->ai_family) {
    case AF_INET: {
        char address[INET_ADDRSTRLEN];
        auto *sockaddr = (struct sockaddr_in *)info_struct->ai_addr;
        inet_ntop(AF_INET, &(sockaddr->sin_addr), address, INET_ADDRSTRLEN);
        return new StringObject { address };
    }
    case AF_INET6: {
        char address[INET6_ADDRSTRLEN];
        auto *sockaddr = (struct sockaddr_in6 *)info_struct->ai_addr;
        inet_ntop(AF_INET6, &(sockaddr->sin6_addr), address, INET6_ADDRSTRLEN);
        return new StringObject { address };
    }
    default:
        NAT_UNREACHABLE();
    }
    return NilObject::the();
}

Value Addrinfo_pfamily(Env *env, Value self, Args args, Block *block) {
    auto addrinfo_void_ptr = self->ivar_get(env, "@_addrinfo"_s)->as_void_p();
    auto info_struct = (struct addrinfo *)addrinfo_void_ptr->void_ptr();
    return Value::integer(info_struct->ai_family);
}

Value Addrinfo_socktype(Env *env, Value self, Args args, Block *block) {
    auto addrinfo_void_ptr = self->ivar_get(env, "@_addrinfo"_s)->as_void_p();
    auto info_struct = (struct addrinfo *)addrinfo_void_ptr->void_ptr();
    return Value::integer(info_struct->ai_socktype);
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
          ary->push(new StringObject((const char*)in->sin6_addr.s6_addr));
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
