#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "natalie.hpp"

using namespace Natalie;

Value init(Env *env, Value self) {
    return NilObject::the();
}

Value Socket_const_name_to_i(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 1, 2);
    auto name = args.at(0);
    bool default_zero = false;
    if (args.size() == 2 && args.at(1)->is_truthy())
        default_zero = true;

    if (!name->is_integer() && !name->is_string() && !name->is_symbol() && name->respond_to(env, "to_str"_s))
        name = name->to_str(env);

    switch (name->type()) {
    case Object::Type::Integer:
        return name;
    case Object::Type::String:
    case Object::Type::Symbol: {
        auto sym = name->to_symbol(env, Object::Conversion::Strict);
        auto Socket = find_top_level_const(env, "Socket"_s);
        auto value = Socket->const_find(env, sym, Object::ConstLookupSearchMode::Strict, Object::ConstLookupFailureMode::Null);
        if (!value)
            value = Socket->const_find(env, "SHORT_CONSTANTS"_s)->as_hash_or_raise(env)->get(env, sym);
        if (!value)
            env->raise_name_error(sym, "uninitialized constant {}::{}", Socket->inspect_str(env), sym->string());
        return value;
    }
    default:
        if (default_zero)
            return Value::integer(0);
        env->raise("TypeError", "{} can't be coerced into String or Integer", name->klass()->inspect_str());
    }
}

static unsigned short Socket_const_get(Env *env, Value name, bool default_zero = false) {
    auto Socket = find_top_level_const(env, "Socket"_s);
    auto value = Socket_const_name_to_i(env, Socket, Args({ name, bool_object(default_zero) }), nullptr);
    return value->as_integer_or_raise(env)->to_nat_int_t();
}

static String Socket_reverse_lookup_address(Env *env, struct sockaddr *addr) {
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    socklen_t length = sizeof(struct sockaddr);

    switch (addr->sa_family) {
    case AF_INET:
        length = sizeof(struct sockaddr_in);
        break;
    case AF_INET6:
        length = sizeof(struct sockaddr_in6);
        break;
    default:
        NAT_NOT_YET_IMPLEMENTED("reverse lookup for family %d", addr->sa_family);
    }

    auto n = getnameinfo(
        addr,
        length,
        hbuf,
        sizeof(hbuf),
        sbuf,
        sizeof(sbuf),
        NI_NAMEREQD);

    if (n != 0)
        env->raise("SocketError", "getnameinfo: {}", gai_strerror(n));

    return hbuf;
}

static int Addrinfo_sockaddr_family(Env *env, StringObject *sockaddr) {
    if (sockaddr->length() == sizeof(struct sockaddr_un)) {
        return AF_UNIX;
    } else {
        return ((struct sockaddr *)(sockaddr->c_str()))->sa_family;
    }
}

Value Addrinfo_initialize(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 1, 4);
    auto sockaddr = args.at(0);
    auto afamily = Socket_const_get(env, args.at(1, NilObject::the()), true);
    auto socktype = Socket_const_get(env, args.at(2, NilObject::the()), true);
    auto protocol = args.at(3, Value::integer(0));

    self->ivar_set(env, "@protocol"_s, protocol);
    self->ivar_set(env, "@socktype"_s, Value::integer(socktype));

    // MRI uses this hack, but I don't really understand why.
    // It gets all the specs to pass though. ¯\_(ツ)_/¯
    bool socktype_hack = false;

    StringObject *unix_path = nullptr;
    IntegerObject *port = nullptr;
    StringObject *host = nullptr;

    if (!afamily)
        self->ivar_set(env, "@pfamily"_s, Value::integer(PF_UNSPEC));

    if (sockaddr->is_string()) {
        if (sockaddr->as_string()->is_empty())
            env->raise("ArgumentError", "bad sockaddr");

        afamily = Addrinfo_sockaddr_family(env, sockaddr->as_string());

        switch (afamily) {
        case AF_UNIX:
            unix_path = GlobalEnv::the()->Object()->const_fetch("Socket"_s).send(env, "unpack_sockaddr_un"_s, { sockaddr })->as_string_or_raise(env);
            break;
        case AF_INET:
        case AF_INET6:
            auto ary = GlobalEnv::the()->Object()->const_fetch("Socket"_s).send(env, "unpack_sockaddr_in"_s, { sockaddr })->as_array_or_raise(env);
            port = ary->at(0)->as_integer_or_raise(env);
            host = ary->at(1)->as_string_or_raise(env);
            break;
        }
    }

    if (sockaddr->is_array()) {
        // initialized with array like ["AF_INET", 49429, "hal", "192.168.0.128"]
        //                          or ["AF_UNIX", "/tmp/sock"]
        auto ary = sockaddr->as_array();
        afamily = Socket_const_get(env, ary->at(0), true);
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
        socktype_hack = true;
    }

    if (afamily == AF_UNIX) {
        assert(unix_path);
        self->ivar_set(env, "@afamily"_s, Value::integer(AF_UNIX));
        self->ivar_set(env, "@unix_path"_s, unix_path);

    } else {
        assert(host);
        assert(port);

        if (host->is_empty())
            host = new StringObject { "0.0.0.0" };

        struct addrinfo hints { };
        struct addrinfo *getaddrinfo_result;

        if (afamily)
            hints.ai_family = afamily;
        else
            hints.ai_family = PF_UNSPEC;

        if (protocol->is_integer())
            hints.ai_protocol = (unsigned short)protocol->as_integer()->to_nat_int_t();
        else
            hints.ai_protocol = 0;

        hints.ai_socktype = socktype;

        if (hints.ai_socktype == 0)
            self->ivar_set(env, "@socktype"_s, Value::integer(0));

        if (socktype_hack && hints.ai_socktype == 0)
            hints.ai_socktype = SOCK_DGRAM;

        const char *service_str = port->to_s(env)->as_string()->c_str();

        switch (hints.ai_socktype) {
        case SOCK_RAW:
            service_str = nullptr;
            hints.ai_protocol = 0;
            break;
        case SOCK_DGRAM:
            if (hints.ai_protocol && hints.ai_protocol != IPPROTO_UDP)
                env->raise("SocketError", "getaddrinfo: {}", gai_strerror(EAI_SOCKTYPE));
            hints.ai_protocol = IPPROTO_UDP;
            break;
        }

        const char *host_str = host->c_str();
        if (host->is_empty())
            host_str = nullptr;

        if (host_str) {
            unsigned char buf[sizeof(struct in6_addr)];
            auto result = inet_pton(hints.ai_family, (const char *)host_str, &buf);
            if (result == 0)
                env->raise("SocketError", "getaddrinfo: nodename nor servname provided, or not known");
            else if (result == -1)
                env->raise_errno();
        }

        int s = getaddrinfo(
            host_str,
            service_str,
            &hints,
            &getaddrinfo_result);
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

Value Addrinfo_to_sockaddr(Env *env, Value self, Args args, Block *block) {
    auto Socket = self->const_find(env, "Socket"_s, Object::ConstLookupSearchMode::NotStrict);

    auto afamily = self->ivar_get(env, "@afamily"_s)->as_integer_or_raise(env)->to_nat_int_t();
    switch (afamily) {
    case AF_UNIX: {
        auto unix_path = self->ivar_get(env, "@unix_path"_s);
        return Socket.send(env, "pack_sockaddr_un"_s, { unix_path });
    }
    case AF_INET:
    case AF_INET6: {
        auto port = self->ivar_get(env, "@ip_port"_s);
        auto address = self->ivar_get(env, "@ip_address"_s);
        return Socket.send(env, "pack_sockaddr_in"_s, { port, address });
    }
    default:
        NAT_NOT_YET_IMPLEMENTED();
    }
}

Value BasicSocket_s_for_fd(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_is(env, 1);
    auto fd = args.at(0);

    auto BasicSocket = find_top_level_const(env, "BasicSocket"_s);

    auto sock = BasicSocket.send(env, "new"_s);
    sock->as_io()->initialize(env, { fd }, block);

    return self;
}

Value BasicSocket_getsockname(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);

    struct sockaddr addr { };
    socklen_t addr_len = sizeof(addr);

    auto getsockname_result = getsockname(
        self->as_io()->fileno(),
        &addr,
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    auto Addrinfo = find_top_level_const(env, "Addrinfo"_s);

    switch (addr.sa_family) {
    case AF_INET: {
        struct sockaddr_in in { };
        socklen_t len = sizeof(in);
        auto getsockname_result = getsockname(
            self->as_io()->fileno(),
            (struct sockaddr *)&in,
            &len);
        if (getsockname_result == -1)
            env->raise_errno();
        return new StringObject { (const char *)&in, len };
    }
    case AF_INET6: {
        struct sockaddr_in in6 { };
        socklen_t len = sizeof(in6);
        auto getsockname_result = getsockname(
            self->as_io()->fileno(),
            (struct sockaddr *)&in6,
            &len);
        if (getsockname_result == -1)
            env->raise_errno();
        return new StringObject { (const char *)&in6, len };
    }
    default:
        NAT_NOT_YET_IMPLEMENTED("BasicSocket#local_address for family %d", addr.sa_family);
    }
}

Value BasicSocket_getsockopt(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_is(env, 2);
    auto level = Socket_const_get(env, args.at(0));
    auto optname = Socket_const_get(env, args.at(1));

    struct sockaddr addr { };
    socklen_t addr_len = sizeof(addr);

    auto getsockname_result = getsockname(
        self->as_io()->fileno(),
        &addr,
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    auto family = addr.sa_family;

    socklen_t len = 1024;
    char buf[len];
    auto result = getsockopt(self->as_io()->fileno(), level, optname, &buf, &len);
    if (result == -1)
        env->raise_errno();
    auto data = new StringObject { buf, len };
    auto Option = fetch_nested_const({ "Socket"_s, "Option"_s });
    return Option.send(env, "new"_s, { Value::integer(family), Value::integer(level), Value::integer(optname), data });
}

Value BasicSocket_local_address(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    auto packed = self.send(env, "getsockname"_s);
    auto Addrinfo = find_top_level_const(env, "Addrinfo"_s);
    return Addrinfo.send(env, "new"_s, { packed });
}

Value BasicSocket_recv(Env *env, Value self, Args args, Block *) {
    // recv(maxlen[, flags[, outbuf]]) => mesg
    args.ensure_argc_between(env, 1, 3);
    auto maxlen = args.at(0)->as_integer_or_raise(env)->to_nat_int_t();
    auto flags = args.at(1, Value::integer(0))->as_integer_or_raise(env)->to_nat_int_t();
    auto outbuf = args.at(2, NilObject::the());

    if (!outbuf->is_nil())
        outbuf->assert_type(env, Object::Type::String, "String");

    if (maxlen <= 0)
        env->raise("ArgumentError", "maxlen must be positive");

    if (flags < 0)
        env->raise("ArgumentError", "flags cannot be negative");

    char buf[maxlen];
    auto bytes = recv(self->as_io()->fileno(), buf, static_cast<size_t>(maxlen), static_cast<int>(flags));
    if (bytes == -1)
        env->raise_errno();

    if (outbuf->is_string())
        outbuf->as_string()->set_str(buf, bytes);

    return new StringObject { buf, static_cast<size_t>(bytes) };
}

Value BasicSocket_send(Env *env, Value self, Args args, Block *) {
    // send(mesg, flags [, dest_sockaddr]) => numbytes_sent
    args.ensure_argc_between(env, 2, 3);
    auto mesg = args.at(0)->to_str(env);
    auto flags = args.at(1, Value::integer(0))->as_integer_or_raise(env)->to_nat_int_t();
    auto dest_sockaddr = args.at(2, NilObject::the());

    const auto bytes = send(self->as_io()->fileno(), mesg->as_string()->c_str(), mesg->as_string()->bytesize(), flags);

    return Value::integer(bytes);
}

Value BasicSocket_setsockopt(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 1, 3);

    unsigned short level = 0;
    unsigned short optname = 0;
    StringObject *data;

    auto Option = fetch_nested_const({ "Socket"_s, "Option"_s });
    if (args.size() == 3) {
        level = Socket_const_get(env, args.at(0));
        optname = Socket_const_get(env, args.at(1));
        auto data_obj = args.at(2);
        switch (data_obj->type()) {
        case Object::Type::String:
            data = data_obj->as_string();
            break;
        case Object::Type::True: {
            int val = 1;
            data = new StringObject { (const char *)(&val), sizeof(int) };
            break;
        }
        case Object::Type::False: {
            int val = 0;
            data = new StringObject { (const char *)(&val), sizeof(int) };
            break;
        }
        case Object::Type::Integer: {
            int val = data_obj->as_integer()->to_nat_int_t();
            data = new StringObject { (const char *)(&val), sizeof(int) };
            break;
        }
        default:
            env->raise("TypeError", "{} can't be coerced into String", data_obj->klass()->inspect_str());
        }
    } else {
        args.ensure_argc_is(env, 1);
        auto option = args.at(0);
        level = option.send(env, "level"_s)->as_integer_or_raise(env)->to_nat_int_t();
        optname = option.send(env, "optname"_s)->as_integer_or_raise(env)->to_nat_int_t();
        data = option.send(env, "data"_s)->as_string_or_raise(env);
    }

    struct sockaddr addr { };
    socklen_t addr_len = sizeof(addr);

    auto getsockname_result = getsockname(
        self->as_io()->fileno(),
        &addr,
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    auto family = addr.sa_family;

    socklen_t len = data->length();
    char buf[len];
    memcpy(buf, data->c_str(), len);
    auto result = setsockopt(self->as_io()->fileno(), level, optname, &buf, len);
    if (result == -1)
        env->raise_errno();
    return Value::integer(result);
}

Value IPSocket_addr(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_between(env, 0, 1);
    auto reverse_lookup = args.at(0, NilObject::the());

    struct sockaddr addr { };
    socklen_t addr_len = sizeof(addr);

    auto getsockname_result = getsockname(
        self->as_io()->fileno(),
        &addr,
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    StringObject *family;
    StringObject *ip;
    StringObject *host;
    Value port;

    if (reverse_lookup->is_nil())
        reverse_lookup = self.send(env, "do_not_reverse_lookup"_s).send(env, "!"_s);
    else if (reverse_lookup == "numeric"_s)
        reverse_lookup = FalseObject::the();
    else if (!reverse_lookup->is_true() && !reverse_lookup->is_false() && reverse_lookup != "hostname"_s)
        env->raise("ArgumentError", "invalid reverse_lookup flag: {}", reverse_lookup->inspect_str(env));

    switch (addr.sa_family) {
    case AF_INET: {
        family = new StringObject("AF_INET");
        struct sockaddr_in in { };
        socklen_t len = sizeof(in);
        auto getsockname_result = getsockname(
            self->as_io()->fileno(),
            (struct sockaddr *)&in,
            &len);
        if (getsockname_result == -1)
            env->raise_errno();
        char host_buf[INET_ADDRSTRLEN];
        auto ntop_result = inet_ntop(AF_INET, &in.sin_addr, host_buf, INET_ADDRSTRLEN);
        if (!ntop_result)
            env->raise_errno();
        host = ip = new StringObject { host_buf };
        port = Value::integer(ntohs(in.sin_port));
        if (reverse_lookup->is_truthy())
            host = new StringObject(Socket_reverse_lookup_address(env, (struct sockaddr *)&in));
        break;
    }
    case AF_INET6: {
        family = new StringObject("AF_INET6");
        struct sockaddr_in in6 { };
        socklen_t len = sizeof(in6);
        auto getsockname_result = getsockname(
            self->as_io()->fileno(),
            (struct sockaddr *)&in6,
            &len);
        if (getsockname_result == -1)
            env->raise_errno();
        char host_buf[INET6_ADDRSTRLEN];
        auto ntop_result = inet_ntop(AF_INET, &in6.sin_addr, host_buf, INET6_ADDRSTRLEN);
        if (!ntop_result)
            env->raise_errno();
        host = ip = new StringObject { host_buf };
        port = Value::integer(ntohs(in6.sin_port));
        if (reverse_lookup->is_truthy())
            host = new StringObject(Socket_reverse_lookup_address(env, (struct sockaddr *)&in6));
        break;
    }
    default:
        NAT_NOT_YET_IMPLEMENTED("IPSocket#addr for family %d", addr.sa_family);
    }

    return new ArrayObject({ family, port, host, ip });
}

Value Socket_initialize(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 2, 3);
    auto afamily = Socket_const_get(env, args.at(0), true);
    auto socktype = Socket_const_get(env, args.at(1), true);
    auto protocol = args.at(2, Value::integer(0))->as_integer_or_raise(env)->to_nat_int_t();

    auto fd = socket(afamily, socktype, protocol);
    if (fd == -1)
        env->raise_errno();

    self->as_io()->initialize(env, { Value::integer(fd) }, block);

    return self;
}

Value Socket_accept(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_is(env, 0);

    if (self->as_io()->is_closed())
        env->raise("IOError", "closed stream");

    socklen_t len = std::max(sizeof(sockaddr_in), sizeof(sockaddr_in6));
    char buf[len];

    auto fd = accept(self->as_io()->fileno(), (struct sockaddr *)&buf, &len);
    if (fd == -1)
        env->raise_errno();

    auto Socket = find_top_level_const(env, "Socket"_s)->as_class_or_raise(env);
    auto socket = new IoObject { Socket };
    socket->as_io()->set_fileno(fd);

    auto Addrinfo = find_top_level_const(env, "Addrinfo"_s);
    auto sockaddr_string = new StringObject { buf, len };
    auto addrinfo = Addrinfo.send(
        env,
        "new"_s,
        {
            sockaddr_string,
            Value::integer(AF_INET),
            Value::integer(SOCK_STREAM),
            Value::integer(0),
        });

    return new ArrayObject({ socket, addrinfo });
}

Value Socket_bind(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_is(env, 1);
    auto sockaddr = args.at(0);

    auto Addrinfo = find_top_level_const(env, "Addrinfo"_s);
    if (!sockaddr->is_a(env, Addrinfo)) {
        if (sockaddr->is_string())
            sockaddr = Addrinfo.send(env, "new"_s, { sockaddr });
        else
            env->raise("TypeError", "expected string or Addrinfo");
    }

    auto Socket = find_top_level_const(env, "Socket"_s);

    auto afamily = sockaddr.send(env, "afamily"_s)->as_integer_or_raise(env)->to_nat_int_t();
    switch (afamily) {
    case AF_INET: {
        struct sockaddr_in addr { };
        auto packed = sockaddr.send(env, "to_sockaddr"_s)->as_string_or_raise(env);
        memcpy(&addr, packed->c_str(), std::min(sizeof(addr), packed->length()));

        auto result = bind(self->as_io()->fileno(), (const struct sockaddr *)&addr, sizeof(addr));
        if (result == -1)
            env->raise_errno();

        auto addr_ary = IPSocket_addr(env, self, {}, nullptr)->as_array_or_raise(env);
        packed = Socket.send(env, "pack_sockaddr_in"_s, { addr_ary->at(1), addr_ary->at(3) }, nullptr)->as_string_or_raise(env);
        sockaddr = Addrinfo.send(env, "new"_s, { packed });

        return Value::integer(result);
    }
    default:
        NAT_NOT_YET_IMPLEMENTED();
    }
}

Value Socket_close(Env *env, Value self, Args args, Block *block) {
    self->as_io()->close(env);
    return NilObject::the();
}

Value Socket_is_closed(Env *env, Value self, Args args, Block *block) {
    return bool_object(self->as_io()->is_closed());
}

Value Socket_connect(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_is(env, 1);
    auto remote_sockaddr = args.at(0)->as_string_or_raise(env);

    auto addr = (struct sockaddr *)remote_sockaddr->c_str();
    socklen_t len = remote_sockaddr->length();

    auto result = connect(self->as_io()->fileno(), addr, len);
    if (result == -1)
        env->raise_errno();

    return Value::integer(0);
}

Value Socket_listen(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_is(env, 1);
    auto backlog = args.at(0)->as_integer_or_raise(env)->to_nat_int_t();

    auto result = listen(self->as_io()->fileno(), backlog);
    if (result == -1)
        env->raise_errno();

    return Value::integer(result);
}

Value Socket_pack_sockaddr_in(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 0, 2);
    auto service = args.at(0, NilObject::the());
    auto host = args.at(1, NilObject::the());
    if (host->is_nil())
        host = new StringObject { "127.0.0.1" };
    if (host->is_string() && host->as_string()->is_empty())
        host = new StringObject { "0.0.0.0" };

    struct addrinfo hints { };
    hints.ai_family = PF_UNSPEC;

    String service_str;
    if (service->is_nil())
        service_str = "";
    else if (service->is_string())
        service_str = service->as_string()->string();
    else if (service->is_integer())
        service_str = service->to_s(env)->string();
    else
        service_str = service->to_str(env)->string();

    struct addrinfo *addr;
    auto result = getaddrinfo(host->as_string_or_raise(env)->c_str(), service_str.c_str(), &hints, &addr);
    if (result != 0)
        env->raise("SocketError", "getaddrinfo: {}", gai_strerror(result));

    StringObject *packed = nullptr;

    switch (addr->ai_family) {
    case AF_INET: {
        auto in = (struct sockaddr_in *)addr->ai_addr;
        packed = new StringObject { (const char *)in, sizeof(struct sockaddr_in) };
        break;
    }
    case AF_INET6: {
        auto in = (struct sockaddr_in6 *)addr->ai_addr;
        packed = new StringObject { (const char *)in, sizeof(struct sockaddr_in6) };
        break;
    }
    default:
        NAT_NOT_YET_IMPLEMENTED("unknown getaddrinfo family");
    }

    freeaddrinfo(addr);

    return packed;
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

    auto family = ((struct sockaddr *)(sockaddr->as_string()->c_str()))->sa_family;

    String sockaddr_string = sockaddr->as_string()->string();
    Value port;
    Value host;

    switch (family) {
    case AF_INET: {
        auto in = (struct sockaddr_in *)sockaddr_string.c_str();
        char host_buf[INET_ADDRSTRLEN];
        auto result = inet_ntop(AF_INET, &in->sin_addr, host_buf, INET_ADDRSTRLEN);
        if (!result)
            env->raise_errno();
        port = Value::integer(ntohs(in->sin_port));
        host = new StringObject(host_buf);
        break;
    }
    case AF_INET6: {
        auto in = (struct sockaddr_in6 *)sockaddr_string.c_str();
        char host_buf[INET6_ADDRSTRLEN];
        auto result = inet_ntop(AF_INET6, &in->sin6_addr, host_buf, INET6_ADDRSTRLEN);
        if (!result)
            env->raise_errno();
        auto ary = new ArrayObject;
        port = Value::integer(ntohs(in->sin6_port));
        host = new StringObject(host_buf);
        break;
    }
    default:
        env->raise("ArgumentError", "not an AF_INET/AF_INET6 sockaddr");
    }

    auto ary = new ArrayObject;
    ary->push(port);
    ary->push(host);
    return ary;
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

static String Socket_family_to_string(int family) {
    switch (family) {
    case AF_INET:
        return "AF_INET";
    case AF_INET6:
        return "AF_INET6";
    default:
        NAT_NOT_YET_IMPLEMENTED("Socket_family_to_string(%d)", family);
    }
}

static int Socket_getaddrinfo_result_port(struct addrinfo *result) {
    switch (result->ai_family) {
    case AF_INET: {
        auto *sockaddr = (struct sockaddr_in *)result->ai_addr;
        return ntohs(sockaddr->sin_port);
    }
    case AF_INET6: {
        auto *sockaddr = (struct sockaddr_in6 *)result->ai_addr;
        return ntohs(sockaddr->sin6_port);
    }
    default:
        NAT_NOT_YET_IMPLEMENTED("port for getaddrinfo result %d", result->ai_family);
    }
}

static String Socket_getaddrinfo_result_host(struct addrinfo *result) {
    switch (result->ai_family) {
    case AF_INET: {
        auto *sockaddr = (struct sockaddr_in *)result->ai_addr;
        char address[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sockaddr->sin_addr), address, INET_ADDRSTRLEN);
        return address;
    }
    case AF_INET6: {
        auto *sockaddr = (struct sockaddr_in6 *)result->ai_addr;
        char address[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(sockaddr->sin6_addr), address, INET6_ADDRSTRLEN);
        return address;
    }
    default:
        NAT_NOT_YET_IMPLEMENTED("port for getaddrinfo result %d", result->ai_family);
    }
}

Value Socket_s_getaddrinfo(Env *env, Value self, Args args, Block *) {
    // getaddrinfo(nodename, servname[, family[, socktype[, protocol[, flags[, reverse_lookup]]]]]) => array
    args.ensure_argc_between(env, 2, 7);
    auto nodename = args.at(0);
    auto servname = args.at(1);
    auto family = args.at(2, NilObject::the());
    auto socktype = args.at(3, NilObject::the());
    auto protocol = args.at(4, NilObject::the());
    auto flags = args.at(5, NilObject::the());
    auto reverse_lookup = args.at(6, NilObject::the());

    if (reverse_lookup->is_nil()) {
        auto BasicSocket = find_top_level_const(env, "BasicSocket"_s);
        reverse_lookup = BasicSocket.send(env, "do_not_reverse_lookup"_s).send(env, "!"_s);
    } else if (reverse_lookup == "numeric"_s) {
        reverse_lookup = FalseObject::the();
    }

    struct addrinfo hints { };
    hints.ai_family = Socket_const_name_to_i(env, self, { family, TrueObject::the() }, nullptr)->as_integer_or_raise(env)->to_nat_int_t();
    hints.ai_socktype = Socket_const_name_to_i(env, self, { socktype, TrueObject::the() }, nullptr)->as_integer_or_raise(env)->to_nat_int_t();
    hints.ai_protocol = Socket_const_name_to_i(env, self, { protocol, TrueObject::the() }, nullptr)->as_integer_or_raise(env)->to_nat_int_t();
    hints.ai_flags = Socket_const_name_to_i(env, self, { flags, TrueObject::the() }, nullptr)->as_integer_or_raise(env)->to_nat_int_t();

    String host;
    String service;

    if (nodename->is_nil() || (nodename->is_string() && nodename->as_string()->is_empty()))
        host = "";
    else if (nodename->is_string())
        host = nodename->as_string_or_raise(env)->string();
    else if (nodename->respond_to(env, "to_str"_s))
        host = nodename->to_str(env)->string();

    if (servname->is_nil() || (servname->is_string() && servname->as_string()->is_empty()))
        service = "0";
    else if (servname->is_integer())
        service = servname->as_integer()->to_s();
    else
        service = servname->as_string_or_raise(env)->string();

    struct addrinfo *result;
    int s = getaddrinfo(
        host.is_empty() ? nullptr : host.c_str(),
        service.c_str(),
        &hints,
        &result);
    if (s != 0)
        env->raise("SocketError", "getaddrinfo: {}", gai_strerror(s));

    auto ary = new ArrayObject;

    do {
        auto addr = new ArrayObject;
        addr->push(new StringObject(Socket_family_to_string(result->ai_family)));
        addr->push(new IntegerObject(Socket_getaddrinfo_result_port(result)));
        if (reverse_lookup->is_truthy())
            addr->push(new StringObject(Socket_reverse_lookup_address(env, result->ai_addr)));
        else
            addr->push(new StringObject(Socket_getaddrinfo_result_host(result)));
        addr->push(new StringObject(Socket_getaddrinfo_result_host(result)));
        addr->push(Value::integer(result->ai_family));
        addr->push(Value::integer(result->ai_socktype));
        addr->push(Value::integer(result->ai_protocol));
        ary->push(addr);
        result = result->ai_next;
    } while (result);

    freeaddrinfo(result);

    return ary;
}

Value Socket_Option_bool(Env *env, Value self, Args, Block *) {
    auto data = self->ivar_get(env, "@data"_s)->as_string_or_raise(env);

    switch (data->length()) {
    case 1:
        return bool_object(data->c_str()[0] != 0);
    case sizeof(int): {
        auto i = *(int *)data->c_str();
        return bool_object(i != 0);
    }
    default:
        return FalseObject::the();
    }
}

Value Socket_Option_int(Env *env, Value self, Args, Block *) {
    auto data = self->ivar_get(env, "@data"_s)->as_string_or_raise(env);

    if (data->length() != sizeof(int))
        return Value::integer(0);

    auto i = *(int *)data->c_str();
    return Value::integer(i);
}

Value Socket_Option_s_linger(Env *env, Value self, Args args, Block *) {
    unsigned short family = 0; // any
    unsigned short level = SOL_SOCKET;
    unsigned short optname = SO_LINGER;

    args.ensure_argc_is(env, 2);
    auto on_off = args.at(0)->is_truthy();
    int linger = args.at(1)->to_int(env)->to_nat_int_t();

    struct linger data {
        on_off, linger
    };

    auto data_string = new StringObject { (const char *)(&data), sizeof(data) };

    return self.send(env, "new"_s, { Value::integer(family), Value::integer(level), Value::integer(optname), data_string });
}

Value Socket_Option_linger(Env *env, Value self, Args, Block *) {
    auto data = self->ivar_get(env, "@data"_s)->as_string_or_raise(env);

    if (data->length() != sizeof(struct linger))
        return Value::integer(0);

    auto l = (struct linger *)data->c_str();
    auto on_off = bool_object(l->l_onoff != 0);
    auto linger = Value::integer(l->l_linger);
    return new ArrayObject({ on_off, linger });
}

Value TCPSocket_initialize(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 2, 5);

    auto host = args.at(0);
    auto port = args.at(1);
    auto local_host = args.at(2, NilObject::the());
    auto local_port = args.at(3, NilObject::the());
    auto connect_timeout = args.keyword_arg(env, "connect_timeout"_s);

    auto fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        env->raise_errno();
    self->as_io()->initialize(env, { Value::integer(fd) }, block);
    self->as_io()->binmode(env);

    auto Socket = find_top_level_const(env, "Socket"_s);

    if (local_host->is_truthy() || local_port->is_truthy()) {
        auto local_sockaddr = Socket.send(env, "pack_sockaddr_in"_s, { local_port, local_host });
        Socket_bind(env, self, { local_sockaddr }, nullptr);
    }

    auto sockaddr = Socket.send(env, "pack_sockaddr_in"_s, { port, host });
    Socket_connect(env, self, { sockaddr }, nullptr);

    if (block) {
        try {
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { self }, nullptr);
            Socket_close(env, self, {}, nullptr);
        } catch (ExceptionObject *exception) {
            Socket_close(env, self, {}, nullptr);
        }
    }

    return self;
}

Value TCPServer_initialize(Env *env, Value self, Args args, Block *block) {
    args.ensure_argc_between(env, 1, 2);
    auto hostname = args.at(0);
    auto port = args.at(1, NilObject::the());

    // TCPServer.new([hostname,] port)
    if (args.size() == 1) {
        port = hostname;
        hostname = new StringObject { "0.0.0.0" };
    }

    auto fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1)
        env->raise_errno();
    self->as_io()->initialize(env, { Value::integer(fd) }, block);

    self.send(env, "setsockopt"_s, { "SOCKET"_s, "REUSEADDR"_s, TrueObject::the() });

    auto Socket = find_top_level_const(env, "Socket"_s);
    auto sockaddr = Socket.send(env, "pack_sockaddr_in"_s, { port, hostname });
    Socket_bind(env, self, { sockaddr }, nullptr);
    Socket_listen(env, self, { Value::integer(1) }, nullptr);

    return self;
}

Value TCPServer_accept(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);

    if (self->as_io()->is_closed())
        env->raise("IOError", "closed stream");

    socklen_t len = std::max(sizeof(sockaddr_in), sizeof(sockaddr_in6));
    char buf[len];

    auto fd = accept(self->as_io()->fileno(), (struct sockaddr *)&buf, &len);
    if (fd == -1)
        env->raise_errno();

    auto TCPSocket = find_top_level_const(env, "TCPSocket"_s)->as_class_or_raise(env);
    auto tcpsocket = new IoObject { TCPSocket };
    tcpsocket->as_io()->set_fileno(fd);

    return tcpsocket;
}
