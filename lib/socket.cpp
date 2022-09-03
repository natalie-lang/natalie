#include "natalie.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

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
