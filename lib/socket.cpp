#include "natalie/integer_methods.hpp"
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

// https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/getifaddrs.3.html
// If both <net/if.h> and <ifaddrs.h> are being included, <net/if.h> must be included before <ifaddrs.h>.
#include <ifaddrs.h>

#ifdef AF_PACKET
#include <netpacket/packet.h>
#endif

#include "natalie.hpp"

using namespace Natalie;

Value init_socket(Env *env, Value self) {
    return Value::nil();
}

Value Socket_const_name_to_i(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 1, 2);
    auto name = args.at(0);
    bool default_zero = false;
    if (args.size() == 2 && args.at(1).is_truthy())
        default_zero = true;

    if (!name.is_integer() && !name.is_string() && !name.is_symbol() && name.respond_to(env, "to_str"_s))
        name = name.to_str(env);

    if (name.is_integer())
        return name;

    if (name.is_string() || name.is_symbol()) {
        auto sym = name.to_symbol(env, Value::Conversion::Strict);
        auto Socket = find_top_level_const(env, "Socket"_s).as_module();
        auto value = Socket->const_find(env, sym, Object::ConstLookupSearchMode::Strict, Object::ConstLookupFailureMode::None);
        if (!value)
            value = Socket->const_find(env, "SHORT_CONSTANTS"_s).value().as_hash_or_raise(env)->get(env, sym);
        if (!value)
            env->raise_name_error(sym, "uninitialized constant {}::{}", Socket->inspect_module(), sym->string());
        return value.value();
    } else {
        if (default_zero)
            return Value::integer(0);
        env->raise("TypeError", "{} can't be coerced into String or Integer", name.klass()->inspect_module());
    }
}

static unsigned short Socket_const_get(Env *env, Value name, bool default_zero = false) {
    auto Socket = find_top_level_const(env, "Socket"_s);
    auto value = Socket_const_name_to_i(env, Socket, Args({ name, bool_object(default_zero) }), nullptr);
    return value.integer_or_raise(env).to_nat_int_t();
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
    case AF_UNIX:
        length = sizeof(struct sockaddr_un);
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

static int blocking_accept(Env *env, IoObject *io, struct sockaddr *addr, socklen_t *len) {
    Defer done_sleeping([] { ThreadObject::set_current_sleeping(false); });
    ThreadObject::set_current_sleeping(true);

    io->select_read(env);
    return ::accept(io->fileno(), addr, len);
}

static Value Server_sysaccept(Env *env, Value self, sockaddr_storage &addr, socklen_t &len, bool is_blocking = true, bool exception = true) {
    if (self.as_io()->is_closed())
        env->raise("IOError", "closed stream");

    int fd;
    if (is_blocking) {
        bool retry = false;
        do {
            retry = false;
            fd = blocking_accept(env, self.as_io(), reinterpret_cast<sockaddr *>(&addr), &len);
            if (fd == -1) {
                if (errno == EAGAIN)
                    retry = true;
                else
                    env->raise_errno();
            }
        } while (retry);
    } else {
        const auto fileno = self.as_io()->fileno();
        self.as_io()->set_nonblock(env, true);
#ifdef __APPLE__
        fd = accept(fileno, reinterpret_cast<sockaddr *>(&addr), &len);
#else
        fd = accept4(fileno, reinterpret_cast<sockaddr *>(&addr), &len, SOCK_CLOEXEC | SOCK_NONBLOCK);
#endif
        if (fd == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                if (!exception)
                    return "wait_readable"_s;
                auto SystemCallError = find_top_level_const(env, "SystemCallError"_s);
                ExceptionObject *error = SystemCallError.send(env, "exception"_s, { Value::integer(errno) }).as_exception();
                auto WaitReadable = fetch_nested_const({ "IO"_s, "WaitReadable"_s }).as_module_or_raise(env);
                error->extend_once(env, WaitReadable);
                env->raise_exception(error);
            } else {
                env->raise_errno();
            }
        }
    }

    return Value::integer(fd);
}

static Value Server_accept(Env *env, Value self, SymbolObject *klass, sockaddr_storage &addr, socklen_t &len, bool is_blocking = true, bool exception = true) {
    auto fd = Server_sysaccept(env, self, addr, len, is_blocking, exception);
    if (!fd.is_integer())
        return fd;

    auto Socket = find_top_level_const(env, klass).as_class_or_raise(env);
    auto socket = IoObject::create(Socket);
    socket->set_fileno(IntegerMethods::convert_to_native_type<int>(env, fd));
    socket->set_close_on_exec(env, Value::True());
    socket->set_nonblock(env, true);
    socket->ivar_set(env, "@do_not_reverse_lookup"_s, find_top_level_const(env, "BasicSocket"_s).send(env, "do_not_reverse_lookup"_s));
    return socket;
}

static int Addrinfo_sockaddr_family(Env *env, StringObject *sockaddr) {
    if (sockaddr->bytesize() < offsetof(struct sockaddr, sa_family) + sizeof(sa_family_t))
        env->raise("ArgumentError", "bad sockaddr");
    return (reinterpret_cast<const struct sockaddr *>(sockaddr->c_str()))->sa_family;
}

Value Addrinfo_getaddrinfo(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_between(env, 2, 6);
    auto nodename = args[0];
    auto servicename = args[1];
    auto family = args.at(2, Value::nil());
    auto socktype = args.at(3, Value::nil());
    auto protocol = args.at(4, Value::nil());
    auto flags = args.at(5, Value::nil());

    const char *node = nullptr;
    const char *service = nullptr;
    addrinfo hints, *res = nullptr;
    memset(&hints, 0, sizeof(hints));

    if (!nodename.is_nil())
        node = nodename.to_str(env)->c_str();
    StringObject *service_as_string = nullptr;
    if (servicename.is_integer()) {
        service_as_string = servicename.to_s(env);
        service = service_as_string->c_str();
    } else if (!servicename.is_nil()) {
        service = servicename.to_str(env)->c_str();
    }
    if (!family.is_nil()) {
        family = Socket_const_name_to_i(env, self, { family }, nullptr);
        hints.ai_family = IntegerMethods::convert_to_native_type<decltype(hints.ai_family)>(env, family);
    }
    if (!socktype.is_nil()) {
        socktype = Socket_const_name_to_i(env, self, { socktype }, nullptr);
        hints.ai_socktype = IntegerMethods::convert_to_native_type<decltype(hints.ai_socktype)>(env, socktype);
    }
    if (!protocol.is_nil()) {
        protocol = Socket_const_name_to_i(env, self, { protocol }, nullptr);
        hints.ai_protocol = IntegerMethods::convert_to_native_type<decltype(hints.ai_protocol)>(env, protocol);
    }
    if (!flags.is_nil()) {
        flags = Socket_const_name_to_i(env, self, { flags }, nullptr);
        hints.ai_flags = IntegerMethods::convert_to_native_type<decltype(hints.ai_flags)>(env, flags);
    }

    const auto s = getaddrinfo(node, service, &hints, &res);
    if (s != 0) {
        if (s == EAI_SYSTEM)
            env->raise_errno();
        auto Socket_ResolutionError = fetch_nested_const({ "Socket"_s, "ResolutionError"_s }).as_class_or_raise(env);
        env->raise(Socket_ResolutionError, "getaddrinfo: {}", gai_strerror(s));
    }
    Defer freeinfo { [&res] { freeaddrinfo(res); } };

    auto output = ArrayObject::create();
    for (struct addrinfo *rp = res; rp != nullptr; rp = rp->ai_next) {
        auto entry = self.send(env, "new"_s, { StringObject::create(reinterpret_cast<const char *>(rp->ai_addr), rp->ai_addrlen, Encoding::ASCII_8BIT), Value::integer(rp->ai_family), Value::integer(rp->ai_socktype), Value::integer(rp->ai_protocol) });
        if (rp->ai_canonname)
            entry->ivar_set(env, "@canonname"_s, StringObject::create(rp->ai_canonname));
        output->push(entry);
    }
    return output;
}

Value Addrinfo_initialize(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_between(env, 1, 4);
    auto sockaddr = args.at(0);
    auto pfamily = Socket_const_get(env, args.at(1, Value::nil()), true);
    auto socktype = Socket_const_get(env, args.at(2, Value::nil()), true);
    auto protocol = args.at(3, Value::integer(0));
    auto afamily = AF_UNSPEC;

    self->ivar_set(env, "@protocol"_s, protocol);
    self->ivar_set(env, "@socktype"_s, Value::integer(socktype));

    // MRI uses this hack, but I don't really understand why.
    // It gets all the specs to pass though. ¯\_(ツ)_/¯
    bool socktype_hack = false;

    StringObject *unix_path = nullptr;
    Optional<Value> port;
    StringObject *host = nullptr;

    if (!pfamily)
        self->ivar_set(env, "@pfamily"_s, Value::integer(PF_UNSPEC));

    if (sockaddr.is_string()) {
        afamily = Addrinfo_sockaddr_family(env, sockaddr.as_string());

        switch (afamily) {
        case AF_UNIX:
            unix_path = GlobalEnv::the()->Object()->const_fetch("Socket"_s).send(env, "unpack_sockaddr_un"_s, { sockaddr }).as_string_or_raise(env);
            break;
        case AF_INET:
        case AF_INET6:
            auto ary = GlobalEnv::the()->Object()->const_fetch("Socket"_s).send(env, "unpack_sockaddr_in"_s, { sockaddr }).as_array_or_raise(env);
            port = ary->at(0).integer_or_raise(env);
            host = ary->at(1).as_string_or_raise(env);
            break;
        }

        self->ivar_set(env, "@pfamily"_s, Value::integer(pfamily));
    }

    if (sockaddr.is_array()) {
        // initialized with array like ["AF_INET", 49429, "hal", "192.168.0.128"]
        //                          or ["AF_UNIX", "/tmp/sock"]
        auto ary = sockaddr.as_array();
        afamily = Socket_const_get(env, ary->ref(env, Value::integer(0)), true);
        self->ivar_set(env, "@afamily"_s, Value::integer(afamily));
        self->ivar_set(env, "@pfamily"_s, Value::integer(afamily));
        switch (afamily) {
        case AF_UNIX:
            unix_path = ary->ref(env, Value::integer(1)).as_string();
            break;
        case AF_INET:
        case AF_INET6:
            port = Value(ary->ref(env, Value::integer(1)).to_int(env));
            host = ary->ref(env, Value::integer(2)).to_str(env);
            if (ary->ref(env, Value::integer(3)).is_string())
                host = ary->at(3).to_str(env);
            break;
        default:
            env->raise("ArgumentError", "bad sockaddr");
        }
        socktype_hack = true;
    }

    if (afamily == PF_UNIX) {
        assert(unix_path);
        self->ivar_set(env, "@afamily"_s, Value::integer(AF_UNIX));
        self->ivar_set(env, "@unix_path"_s, unix_path);

    } else {
        assert(host);
        assert(port.present());

        if (host->is_empty())
            host = StringObject::create("0.0.0.0");

        struct addrinfo hints { };
        struct addrinfo *getaddrinfo_result = nullptr;

        if (afamily)
            hints.ai_family = afamily;
        else
            hints.ai_family = AF_UNSPEC;

        if (protocol.is_integer())
            hints.ai_protocol = (unsigned short)protocol.integer().to_nat_int_t();
        else
            hints.ai_protocol = 0;

        hints.ai_socktype = socktype;

        if (hints.ai_socktype == 0)
            self->ivar_set(env, "@socktype"_s, Value::integer(0));

        if (socktype_hack && hints.ai_socktype == 0)
            hints.ai_socktype = SOCK_DGRAM;

        const char *service_str = IntegerMethods::to_s(env, port.value().integer()).as_string()->c_str();

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

        Defer freeinfo { [&getaddrinfo_result] { freeaddrinfo(getaddrinfo_result); } };

        if (self->ivar_get(env, "@pfamily"_s).is_nil())
            self->ivar_set(env, "@pfamily"_s, Value::integer(getaddrinfo_result->ai_family));

        if (self->ivar_get(env, "@socktype"_s).is_nil())
            self->ivar_set(env, "@socktype"_s, Value::integer(getaddrinfo_result->ai_socktype));

        if (getaddrinfo_result->ai_family != afamily)
            env->raise("SocketError", "getaddrinfo: Address family for hostname not supported");

        switch (getaddrinfo_result->ai_family) {
        case AF_INET: {
            self->ivar_set(env, "@afamily"_s, Value::integer(AF_INET));
            char address[INET_ADDRSTRLEN];
            auto *sockaddr = reinterpret_cast<sockaddr_in *>(getaddrinfo_result->ai_addr);
            inet_ntop(AF_INET, &(sockaddr->sin_addr), address, INET_ADDRSTRLEN);
            self->ivar_set(env, "@ip_address"_s, StringObject::create(address));
            auto port_in_network_byte_order = sockaddr->sin_port;
            self->ivar_set(env, "@ip_port"_s, Value::integer(ntohs(port_in_network_byte_order)));
            break;
        }
        case AF_INET6: {
            self->ivar_set(env, "@afamily"_s, Value::integer(AF_INET6));
            char address[INET6_ADDRSTRLEN];
            auto *sockaddr = reinterpret_cast<sockaddr_in6 *>(getaddrinfo_result->ai_addr);
            inet_ntop(AF_INET6, &(sockaddr->sin6_addr), address, INET6_ADDRSTRLEN);
            self->ivar_set(env, "@ip_address"_s, StringObject::create(address));
            auto port_in_network_byte_order = sockaddr->sin6_port;
            self->ivar_set(env, "@ip_port"_s, Value::integer(ntohs(port_in_network_byte_order)));
            break;
        }
        default:
            env->raise("SocketError", "getaddrinfo: unrecognized result");
        }
    }

    return self;
}

Value Addrinfo_getnameinfo(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 0, 1);
    const auto flags = IntegerMethods::convert_to_native_type<int>(env, args.at(0, Value::integer(0)));
    auto sockaddr = self.send(env, "to_sockaddr"_s).as_string();
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    const auto res = getnameinfo(reinterpret_cast<const struct sockaddr *>(sockaddr->c_str()), sockaddr->bytesize(), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), flags);
    if (res < 0) {
        if (res == EAI_SYSTEM)
            env->raise_errno();
        env->raise("SocketError", "getnameinfo: {}", gai_strerror(res));
    }
    return ArrayObject::create({
        StringObject::create(hbuf, Encoding::ASCII_8BIT),
        StringObject::create(sbuf, Encoding::ASCII_8BIT),
    });
}

Value Addrinfo_to_sockaddr(Env *env, Value self, Args &&args, Block *block) {
    auto Socket = find_top_level_const(env, "Socket"_s);

    auto afamily = self->ivar_get(env, "@afamily"_s).integer_or_raise(env).to_nat_int_t();
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

Value BasicSocket_s_for_fd(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 1);
    auto fd = args.at(0);

    auto sock = Object::allocate(env, self.as_class(), {}, nullptr);
    sock.as_io()->initialize(env, { fd }, block);
    sock.as_io()->binmode(env);

    return sock;
}

Value BasicSocket_close_read(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);
    if (self.as_io()->is_closed())
        env->raise("IOError", "closed stream");
    auto read_closed = self->ivar_get(env, "@read_closed"_s);
    if (read_closed.is_truthy())
        return Value::nil();
    auto write_closed = self->ivar_get(env, "@write_closed"_s);
    if (write_closed.is_truthy()) {
        self.send(env, "close"_s);
    } else {
        shutdown(self.as_io()->fileno(), SHUT_RD);
    }
    self->ivar_set(env, "@read_closed"_s, Value::True());
    return Value::nil();
}

Value BasicSocket_close_write(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);
    if (self.as_io()->is_closed())
        env->raise("IOError", "closed stream");
    auto write_closed = self->ivar_get(env, "@write_closed"_s);
    if (write_closed.is_truthy())
        return Value::nil();
    auto read_closed = self->ivar_get(env, "@read_closed"_s);
    if (read_closed.is_truthy()) {
        self.send(env, "close"_s);
    } else {
        shutdown(self.as_io()->fileno(), SHUT_WR);
    }
    self->ivar_set(env, "@write_closed"_s, Value::True());
    return Value::nil();
}

Value BasicSocket_getpeername(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    sockaddr_storage addr {};
    socklen_t addr_len = sizeof(addr);

    auto getpeername_result = getpeername(
        self.as_io()->fileno(),
        reinterpret_cast<sockaddr *>(&addr),
        &addr_len);
    if (getpeername_result == -1)
        env->raise_errno();

    return StringObject::create(reinterpret_cast<const char *>(&addr), addr_len, Encoding::ASCII_8BIT);
}

Value BasicSocket_getsockname(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    sockaddr_storage addr {};
    socklen_t addr_len = sizeof(addr);

    auto getsockname_result = getsockname(
        self.as_io()->fileno(),
        reinterpret_cast<sockaddr *>(&addr),
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    return StringObject::create(reinterpret_cast<const char *>(&addr), addr_len, Encoding::ASCII_8BIT);
}

Value BasicSocket_getsockopt(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 2);
    auto level = Socket_const_get(env, args.at(0));
    auto optname_val = args.at(1);
    if (optname_val == "CORK"_s || (optname_val.is_string() && *optname_val.as_string() == "CORK"))
        optname_val = level == IPPROTO_UDP ? "UDP_CORK"_s : "TCP_CORK"_s;
    auto optname = Socket_const_get(env, optname_val);

    struct sockaddr addr { };
    socklen_t addr_len = sizeof(addr);

    auto getsockname_result = getsockname(
        self.as_io()->fileno(),
        &addr,
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    auto family = addr.sa_family;

    socklen_t len = 1024;
    String str { len, '\0' };
    auto result = getsockopt(self.as_io()->fileno(), level, optname, &str[0], &len);
    if (result == -1)
        env->raise_errno();
    str.truncate(len);
    auto data = StringObject::create(std::move(str), Encoding::ASCII_8BIT);
    auto Option = fetch_nested_const({ "Socket"_s, "Option"_s });
    return Option.send(env, "new"_s, { Value::integer(family), Value::integer(level), Value::integer(optname), data });
}

Value BasicSocket_local_address(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    sockaddr_storage addr {};
    socklen_t addr_len = sizeof(addr);

    auto getsockname_result = getsockname(
        self.as_io()->fileno(),
        reinterpret_cast<sockaddr *>(&addr),
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    auto packed = StringObject::create(reinterpret_cast<const char *>(&addr), addr_len, Encoding::ASCII_8BIT);

    int socktype = 0;
    socklen_t socktype_len = sizeof(socktype);
    if (getsockopt(self.as_io()->fileno(), SOL_SOCKET, SO_TYPE, &socktype, &socktype_len) == -1)
        env->raise_errno();

    int protocol = 0;
    if (addr.ss_family == AF_INET || addr.ss_family == AF_INET6) {
        if (socktype == SOCK_STREAM) {
            protocol = IPPROTO_TCP;
        } else if (socktype == SOCK_DGRAM) {
            protocol = IPPROTO_UDP;
        }
    }

    auto Addrinfo = find_top_level_const(env, "Addrinfo"_s);
    return Addrinfo.send(env, "new"_s, { packed, Value::integer(addr.ss_family), Value::integer(socktype), Value::integer(protocol) });
}

static ssize_t blocking_recv(Env *env, IoObject *io, char *buf, size_t len, int flags) {
    Defer done_sleeping([] { ThreadObject::set_current_sleeping(false); });
    ThreadObject::set_current_sleeping(true);

    io->select_read(env);
    return ::recv(io->fileno(), buf, len, flags);
}

Value BasicSocket_recv(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 1, 3);
    auto maxlen = args.at(0).integer_or_raise(env).to_nat_int_t();
    auto flags = args.at(1, Value::integer(0)).integer_or_raise(env).to_nat_int_t();
    auto outbuf = args.at(2, Value::nil());

    if (!outbuf.is_nil())
        outbuf.assert_type(env, Object::Type::String, "String");

    if (maxlen <= 0)
        env->raise("ArgumentError", "maxlen must be positive");

    if (flags < 0)
        env->raise("ArgumentError", "flags cannot be negative");

    Defer done_sleeping([] { ThreadObject::set_current_sleeping(false); });
    ThreadObject::set_current_sleeping(true);

    String str { static_cast<size_t>(maxlen), '\0' };
    auto bytes = blocking_recv(env, self.as_io(), &str[0], static_cast<size_t>(maxlen), static_cast<int>(flags));
    if (bytes == -1)
        env->raise_errno();

    str.truncate(bytes);

    if (outbuf.is_string()) {
        outbuf.as_string()->set_str(std::move(str));
        return outbuf;
    }

    return StringObject::create(std::move(str));
}

Value BasicSocket_recv_nonblock(Env *env, Value self, Args &&args, Block *) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_between(env, 1, 3);

    const auto maxlen = IntegerMethods::convert_to_nat_int_t(env, args[0]);
    const auto flags = IntegerMethods::convert_to_nat_int_t(env, args.at(1, Value::integer(0)));
    auto buffer = args.at(2, StringObject::create("", Encoding::ASCII_8BIT)).to_str(env);
    auto exception = kwargs ? kwargs->remove(env, "exception"_s) : Value::True();
    env->ensure_no_extra_keywords(kwargs);

#ifdef __APPLE__
    self.as_io()->set_nonblock(env, true);
#endif
    char charbuf[maxlen];
    const auto recvfrom_result = recvfrom(
        self.as_io()->fileno(),
        charbuf, maxlen,
        flags | MSG_DONTWAIT,
        nullptr, nullptr);
    if (recvfrom_result < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            if (exception && exception.value().is_falsey())
                return "wait_readable"_s;
            auto SystemCallError = find_top_level_const(env, "SystemCallError"_s);
            ExceptionObject *error = SystemCallError.send(env, "exception"_s, { Value::integer(errno) }).as_exception();
            auto WaitReadable = fetch_nested_const({ "IO"_s, "WaitReadable"_s }).as_module_or_raise(env);
            error->extend_once(env, WaitReadable);
            env->raise_exception(error);
        } else {
            env->raise_errno();
        }
    }

    buffer->set_str(charbuf, recvfrom_result);
    return buffer;
}

Value BasicSocket_remote_address(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    sockaddr_storage addr {};
    socklen_t addr_len = sizeof(addr);

    auto getsockname_result = getpeername(
        self.as_io()->fileno(),
        reinterpret_cast<sockaddr *>(&addr),
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    auto packed = StringObject::create(reinterpret_cast<const char *>(&addr), addr_len, Encoding::ASCII_8BIT);

    int socktype = 0;
    socklen_t socktype_len = sizeof(socktype);
    if (getsockopt(self.as_io()->fileno(), SOL_SOCKET, SO_TYPE, &socktype, &socktype_len) == -1)
        env->raise_errno();

    int protocol = 0;
    if (addr.ss_family == AF_INET || addr.ss_family == AF_INET6) {
        if (socktype == SOCK_STREAM) {
            protocol = IPPROTO_TCP;
        } else if (socktype == SOCK_DGRAM) {
            protocol = IPPROTO_UDP;
        }
    }

    auto Addrinfo = find_top_level_const(env, "Addrinfo"_s);
    return Addrinfo.send(env, "new"_s, { packed, Value::integer(addr.ss_family), Value::integer(socktype), Value::integer(protocol) });
}

Value BasicSocket_send(Env *env, Value self, Args &&args, Block *) {
    // send(mesg, flags [, dest_sockaddr]) => numbytes_sent
    args.ensure_argc_between(env, 2, 3);
    auto mesg = args.at(0).to_str(env);
    auto flags = args.at(1, Value::integer(0)).integer_or_raise(env).to_nat_int_t();
    auto dest_sockaddr = args.at(2, Value::nil());
    ssize_t bytes;

    if (dest_sockaddr.is_nil()) {
        bytes = send(self.as_io()->fileno(), mesg->c_str(), mesg->bytesize(), flags);
    } else {
        auto Addrinfo = find_top_level_const(env, "Addrinfo"_s);
        if (dest_sockaddr.is_a(env, Addrinfo))
            dest_sockaddr = dest_sockaddr.to_s(env);
        dest_sockaddr = dest_sockaddr.to_str(env);
        bytes = sendto(self.as_io()->fileno(), mesg->c_str(), mesg->bytesize(), flags, reinterpret_cast<const sockaddr *>(dest_sockaddr.as_string()->c_str()), dest_sockaddr.as_string()->bytesize());
    }
    if (bytes < 0)
        env->raise_errno();

    return Value::integer(bytes);
}

Value BasicSocket_setsockopt(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_between(env, 1, 3);

    unsigned short level = 0;
    unsigned short optname = 0;
    StringObject *data;

    auto Option = fetch_nested_const({ "Socket"_s, "Option"_s });
    if (args.size() == 3) {
        level = Socket_const_get(env, args.at(0));
        optname = Socket_const_get(env, args.at(1));
        auto data_obj = args.at(2);
        if (data_obj.is_integer()) {
            int val = data_obj.integer().to_nat_int_t();
            data = StringObject::create((const char *)(&val), sizeof(int));
        } else if (data_obj.is_string()) {
            data = data_obj.as_string();
        } else if (data_obj.is_true()) {
            int val = 1;
            data = StringObject::create((const char *)(&val), sizeof(int));
        } else if (data_obj.is_false()) {
            int val = 0;
            data = StringObject::create((const char *)(&val), sizeof(int));
        } else {
            env->raise("TypeError", "{} can't be coerced into String", data_obj.klass()->inspect_module());
        }
    } else {
        args.ensure_argc_is(env, 1);
        auto option = args.at(0);
        level = option.send(env, "level"_s).integer_or_raise(env).to_nat_int_t();
        optname = option.send(env, "optname"_s).integer_or_raise(env).to_nat_int_t();
        data = option.send(env, "data"_s).as_string_or_raise(env);
    }

    struct sockaddr addr { };
    socklen_t addr_len = sizeof(addr);

    auto getsockname_result = getsockname(
        self.as_io()->fileno(),
        &addr,
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    auto family = addr.sa_family;

    socklen_t len = data->length();
    char *buf = static_cast<char *>(alloca(len));
    memcpy(buf, data->c_str(), len);
    auto result = setsockopt(self.as_io()->fileno(), level, optname, buf, len);
    if (result == -1)
        env->raise_errno();
    return Value::integer(result);
}

Value BasicSocket_shutdown(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 0, 1);

    int how = SHUT_RDWR;
    if (args.size() > 0) {
        auto arg = args.at(0);
        if (arg.is_integer()) {
            how = arg.integer().to_nat_int_t();
            switch (how) {
            case SHUT_RD:
            case SHUT_WR:
            case SHUT_RDWR:
                break;
            default:
                env->raise("ArgumentError", "invalid shutdown mode: {}", how);
            }
        } else {
            auto how_sym = arg.to_symbol(env, Value::Conversion::Strict);
            if (how_sym == "RD"_s || how_sym == "SHUT_RD"_s)
                how = SHUT_RD;
            else if (how_sym == "WR"_s || how_sym == "SHUT_WR"_s)
                how = SHUT_WR;
            else if (how_sym == "RDWR"_s || how_sym == "SHUT_RDWR"_s)
                how = SHUT_RDWR;
            else
                env->raise("SocketError", "invalid shutdown mode: {}", how_sym->string());
        }
    }

    auto result = ::shutdown(self.as_io()->fileno(), how);
    if (result == -1)
        env->raise_errno();

    return Value::nil();
}

Value IPSocket_addr(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 0, 1);
    auto reverse_lookup = args.at(0, Value::nil());

    sockaddr_storage addr {};
    socklen_t addr_len = sizeof(addr);

    auto getsockname_result = getsockname(
        self.as_io()->fileno(),
        reinterpret_cast<sockaddr *>(&addr),
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    StringObject *family;
    StringObject *ip;
    StringObject *host;
    Value port;

    if (reverse_lookup.is_nil())
        reverse_lookup = self.send(env, "do_not_reverse_lookup"_s).send(env, "!"_s);
    else if (reverse_lookup == "numeric"_s)
        reverse_lookup = Value::False();
    else if (!reverse_lookup.is_true() && !reverse_lookup.is_false() && reverse_lookup != "hostname"_s)
        env->raise("ArgumentError", "invalid reverse_lookup flag: {}", reverse_lookup.inspected(env));

    switch (addr.ss_family) {
    case AF_INET: {
        family = StringObject::create("AF_INET");
        const auto *in = reinterpret_cast<sockaddr_in *>(&addr);
        char host_buf[INET_ADDRSTRLEN];
        auto ntop_result = inet_ntop(addr.ss_family, &in->sin_addr, host_buf, INET_ADDRSTRLEN);
        if (!ntop_result)
            env->raise_errno();
        host = ip = StringObject::create(host_buf);
        port = Value::integer(ntohs(in->sin_port));
        if (reverse_lookup.is_truthy())
            host = StringObject::create(Socket_reverse_lookup_address(env, reinterpret_cast<sockaddr *>(&addr)));
        break;
    }
    case AF_INET6: {
        family = StringObject::create("AF_INET6");
        const auto *in6 = reinterpret_cast<sockaddr_in6 *>(&addr);
        char host_buf[INET6_ADDRSTRLEN];
        auto ntop_result = inet_ntop(addr.ss_family, &in6->sin6_addr, host_buf, INET6_ADDRSTRLEN);
        if (!ntop_result)
            env->raise_errno();
        host = ip = StringObject::create(host_buf);
        port = Value::integer(ntohs(in6->sin6_port));
        if (reverse_lookup.is_truthy())
            host = StringObject::create(Socket_reverse_lookup_address(env, reinterpret_cast<sockaddr *>(&addr)));
        break;
    }
    default:
        NAT_NOT_YET_IMPLEMENTED("IPSocket#addr for family %d", addr.ss_family);
    }

    return ArrayObject::create({ family, port, host, ip });
}

Value IPSocket_peeraddr(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 0, 1);
    auto reverse_lookup = args.at(0, Value::nil());

    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    if (reverse_lookup.is_nil()) {
        reverse_lookup = self.send(env, "do_not_reverse_lookup"_s).send(env, "!"_s);
    } else if (reverse_lookup == "numeric"_s) {
        reverse_lookup = Value::False();
    } else if (!reverse_lookup.is_true() && !reverse_lookup.is_false() && reverse_lookup != "hostname"_s) {
        env->raise("ArgumentError", "invalid reverse_lookup flag: {}", reverse_lookup.inspected(env));
    }

    auto getsockname_result = getpeername(
        self.as_io()->fileno(),
        reinterpret_cast<struct sockaddr *>(&addr),
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    StringObject *family;
    StringObject *ip;
    StringObject *host;
    Value port;
    switch (addr.ss_family) {
    case AF_INET: {
        family = StringObject::create("AF_INET");
        const auto *in = reinterpret_cast<sockaddr_in *>(&addr);
        char host_buf[INET_ADDRSTRLEN];
        auto ntop_result = inet_ntop(addr.ss_family, &in->sin_addr, host_buf, INET_ADDRSTRLEN);
        if (!ntop_result)
            env->raise_errno();
        host = ip = StringObject::create(host_buf);
        port = Value::integer(ntohs(in->sin_port));
        if (reverse_lookup.is_truthy())
            host = StringObject::create(Socket_reverse_lookup_address(env, reinterpret_cast<sockaddr *>(&addr)));
        break;
    }
    case AF_INET6: {
        family = StringObject::create("AF_INET6");
        const auto *in6 = reinterpret_cast<sockaddr_in6 *>(&addr);
        char host_buf[INET6_ADDRSTRLEN];
        auto ntop_result = inet_ntop(addr.ss_family, &in6->sin6_addr, host_buf, INET6_ADDRSTRLEN);
        if (!ntop_result)
            env->raise_errno();
        host = ip = StringObject::create(host_buf);
        port = Value::integer(ntohs(in6->sin6_port));
        if (reverse_lookup.is_truthy())
            host = StringObject::create(Socket_reverse_lookup_address(env, reinterpret_cast<sockaddr *>(&addr)));
        break;
    }
    default:
        NAT_NOT_YET_IMPLEMENTED("IPSocket#addr for family %d", addr.ss_family);
    }

    return ArrayObject::create({ family, port, host, ip });
}

Value IPSocket_recvfrom(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 1, 2);
    const auto size = IntegerMethods::convert_to_nat_int_t(env, args[0]);
    const auto flags = IntegerMethods::convert_to_nat_int_t(env, args.at(1, Value::integer(0)));

    TM::String buf { static_cast<size_t>(size), '\0' };
    sockaddr_storage addr {};
    socklen_t addr_len = sizeof(addr);

    const auto recvfrom_result = recvfrom(
        self.as_io()->fileno(),
        &buf[0], buf.size(),
        flags,
        reinterpret_cast<struct sockaddr *>(&addr), &addr_len);
    if (recvfrom_result < 0)
        env->raise_errno();

    if (static_cast<size_t>(recvfrom_result) < buf.size())
        buf.truncate(recvfrom_result);

    auto ipaddr_info = IPSocket_addr(env, self, {}, nullptr);
    // We need the other port
    switch (addr.ss_family) {
    case AF_INET: {
        auto addr_in = reinterpret_cast<sockaddr_in *>(&addr);
        ipaddr_info.as_array()->operator[](1) = Value::integer(ntohs(addr_in->sin_port));
        break;
    }
    case AF_INET6: {
        auto addr_in6 = reinterpret_cast<sockaddr_in6 *>(&addr);
        ipaddr_info.as_array()->operator[](1) = Value::integer(ntohs(addr_in6->sin6_port));
        break;
    }
    default:
        // Some issue, which means we should replace the ipaddr info
        ipaddr_info = Value::nil();
    }
    return ArrayObject::create({
        StringObject::create(std::move(buf), Encoding::ASCII_8BIT),
        ipaddr_info,
    });
}

Value UNIXSocket_addr(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 0);

    struct sockaddr_un addr { };
    socklen_t addr_len = sizeof(addr);

    auto getsockname_result = getsockname(
        self.as_io()->fileno(),
        reinterpret_cast<struct sockaddr *>(&addr),
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    return ArrayObject::create({ StringObject::create("AF_UNIX"),
        StringObject::create(addr.sun_path) });
}

Value UNIXSocket_peeraddr(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 0);

    struct sockaddr_un addr { };
    socklen_t addr_len = sizeof(addr);

    auto getsockname_result = getpeername(
        self.as_io()->fileno(),
        reinterpret_cast<struct sockaddr *>(&addr),
        &addr_len);
    if (getsockname_result == -1)
        env->raise_errno();

    return ArrayObject::create({ StringObject::create("AF_UNIX"),
        StringObject::create(addr.sun_path) });
}

Value UNIXSocket_recvfrom(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 1, 3);
    const auto size = IntegerMethods::convert_to_nat_int_t(env, args[0]);
    const auto flags = IntegerMethods::convert_to_nat_int_t(env, args.at(1, Value::integer(0)));
    StringObject *result = nullptr;
    if (args.size() > 2) {
        result = args[2].to_str(env);
    } else {
        result = StringObject::create("", Encoding::ASCII_8BIT);
    }

    TM::String buf { static_cast<size_t>(size), '\0' };
    struct sockaddr_un addr { };
    socklen_t addr_len = sizeof(addr);

    const auto recvfrom_result = recvfrom(
        self.as_io()->fileno(),
        &buf[0], buf.size(),
        flags,
        reinterpret_cast<struct sockaddr *>(&addr), &addr_len);
    if (recvfrom_result < 0)
        env->raise_errno();

    if (static_cast<size_t>(recvfrom_result) < buf.size())
        buf.truncate(recvfrom_result);

    result->set_str(std::move(buf));

    auto unixaddress = ArrayObject::create({ StringObject::create("AF_UNIX"), StringObject::create(addr.sun_path) });
    return ArrayObject::create({ result, unixaddress });
}

Value UNIXSocket_pair(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_between(env, 0, 2);
    // NATFIXME: Add support for symbolized type (like :SOCK_STREAM)
    const auto type = IntegerMethods::convert_to_nat_int_t(env, args.at(0, Value::integer(SOCK_STREAM)));
    const auto protocol = IntegerMethods::convert_to_nat_int_t(env, args.at(1, Value::integer(0)));

    int fd[2];
    if (socketpair(AF_UNIX, type, protocol, fd) < 0)
        env->raise_errno();

    return ArrayObject::create({ self.send(env, "for_fd"_s, { Value::integer(fd[0]) }),
        self.send(env, "for_fd"_s, { Value::integer(fd[1]) }) });
}

Value Socket_initialize(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_between(env, 2, 3);
    auto afamily = Socket_const_get(env, args.at(0), true);
    auto socktype = Socket_const_get(env, args.at(1), true);
    auto protocol = args.at(2, Value::integer(0)).integer_or_raise(env).to_nat_int_t();

    auto fd = socket(afamily, socktype, protocol);
    if (fd == -1)
        env->raise_errno();

    self.as_io()->initialize(env, { Value::integer(fd) }, block);
    self.as_io()->binmode(env);
    self->ivar_set(env, "@do_not_reverse_lookup"_s, find_top_level_const(env, "BasicSocket"_s).send(env, "do_not_reverse_lookup"_s));

    return self;
}

static Value Socket_accept(Env *env, Value socket, sockaddr_storage &addr, socklen_t &len) {
    auto Addrinfo = find_top_level_const(env, "Addrinfo"_s);
    auto sockaddr_string = StringObject::create(reinterpret_cast<char *>(&addr), len, Encoding::ASCII_8BIT);
    auto addrinfo = Addrinfo.send(
        env,
        "new"_s,
        {
            sockaddr_string,
            Value::integer(addr.ss_family),
            Value::integer(SOCK_STREAM),
            Value::integer(0),
        });

    return ArrayObject::create({ socket, addrinfo });
}

Value Socket_accept(Env *env, Value self, bool blocking = true, bool exception = false) {
    sockaddr_storage addr {};
    socklen_t len = sizeof(addr);
    auto socket = Server_accept(env, self, "Socket"_s, addr, len, blocking, exception);
    if (socket.is_symbol())
        return socket;
    return Socket_accept(env, socket, addr, len);
}

Value Socket_accept(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 0);
    return Socket_accept(env, self, true);
}

Value Socket_accept_nonblock(Env *env, Value self, Args &&args, Block *block) {
    auto kwargs = args.pop_keyword_hash();
    auto exception = kwargs ? kwargs->remove(env, "exception"_s) : Value::True();
    env->ensure_no_extra_keywords(kwargs);
    args.ensure_argc_is(env, 0);
    return Socket_accept(env, self, false, exception.value().is_truthy());
}

Value Socket_bind(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 1);
    auto sockaddr = args.at(0);

    auto Addrinfo = find_top_level_const(env, "Addrinfo"_s);
    if (!sockaddr.is_a(env, Addrinfo)) {
        if (sockaddr.is_string())
            sockaddr = Addrinfo.send(env, "new"_s, { sockaddr });
        else
            env->raise("TypeError", "expected string or Addrinfo");
    }

    auto Socket = find_top_level_const(env, "Socket"_s);

    auto afamily = sockaddr.send(env, "afamily"_s).integer_or_raise(env).to_nat_int_t();
    switch (afamily) {
    case AF_INET: {
        struct sockaddr_in addr { };
        auto packed = sockaddr.send(env, "to_sockaddr"_s).as_string_or_raise(env);
        memcpy(&addr, packed->c_str(), std::min(sizeof(addr), packed->length()));

        auto result = bind(self.as_io()->fileno(), reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr));
        if (result == -1)
            env->raise_errno();

        auto addr_ary = IPSocket_addr(env, self, {}, nullptr).as_array_or_raise(env);
        packed = Socket.send(env, "pack_sockaddr_in"_s, { addr_ary->at(1), addr_ary->at(3) }, nullptr).as_string_or_raise(env);
        sockaddr = Addrinfo.send(env, "new"_s, { packed });

        return Value::integer(result);
    }
    case AF_INET6: {
        struct sockaddr_in6 addr { };
        auto packed = sockaddr.send(env, "to_sockaddr"_s).as_string_or_raise(env);
        memcpy(&addr, packed->c_str(), std::min(sizeof(addr), packed->length()));

        auto result = bind(self.as_io()->fileno(), reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr));
        if (result == -1)
            env->raise_errno();

        auto addr_ary = IPSocket_addr(env, self, {}, nullptr).as_array_or_raise(env);
        packed = Socket.send(env, "pack_sockaddr_in"_s, { addr_ary->at(1), addr_ary->at(3) }, nullptr).as_string_or_raise(env);
        sockaddr = Addrinfo.send(env, "new"_s, { packed });

        return Value::integer(result);
    }
    case AF_UNIX: {
        struct sockaddr_un addr { };
        auto packed = sockaddr.send(env, "to_sockaddr"_s).as_string_or_raise(env);
        memcpy(&addr, packed->c_str(), std::min(sizeof(addr), packed->length()));

        auto result = bind(self.as_io()->fileno(), reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr));
        if (result == -1)
            env->raise_errno();

        auto addr_ary = UNIXSocket_addr(env, self, {}, nullptr).as_array_or_raise(env);
        packed = Socket.send(env, "pack_sockaddr_un"_s, { addr_ary->at(1) }, nullptr).as_string_or_raise(env);
        sockaddr = Addrinfo.send(env, "new"_s, { packed });

        return Value::integer(result);
    }
    default:
        NAT_NOT_YET_IMPLEMENTED();
    }
}

Value Socket_close(Env *env, Value self, Args &&args, Block *block) {
    self.as_io()->close(env);
    return Value::nil();
}

Value Socket_is_closed(Env *env, Value self, Args &&args, Block *block) {
    return bool_object(self.as_io()->is_closed());
}

Value Socket_connect(Env *env, Value self, Args &&args, Block *block) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_is(env, 1);
    auto remote_sockaddr = args.at(0);
    auto connect_timeout = kwargs ? kwargs->remove(env, "connect_timeout"_s) : Optional<Value> {};
    env->ensure_no_extra_keywords(kwargs);

    auto Addrinfo = find_top_level_const(env, "Addrinfo"_s);
    if (remote_sockaddr.is_a(env, Addrinfo)) {
        remote_sockaddr = remote_sockaddr.to_s(env);
    } else {
        remote_sockaddr = remote_sockaddr.to_str(env);
    }

    auto addr = reinterpret_cast<const sockaddr *>(remote_sockaddr.as_string()->c_str());
    socklen_t len = remote_sockaddr.as_string()->bytesize();

    auto result = connect(self.as_io()->fileno(), addr, len);
    if (result == -1) {
        if (errno != EINPROGRESS)
            env->raise_errno();

        int option_value = 0;
        socklen_t option_len = sizeof(option_value);
        if (getsockopt(self.as_io()->fileno(), SOL_SOCKET, SO_ERROR, &option_value, &option_len) == -1)
            env->raise_errno();
        if (option_value)
            env->raise_errno(option_value);

        // We can't connect directly, wait for the timeout (if given)
        pollfd fds = { self.as_io()->fileno(), POLLIN | POLLOUT, 0 };
        int timeout = -1;
        if (connect_timeout && !connect_timeout.value().is_nil()) {
            const auto timeout_f = connect_timeout.value().to_f(env)->to_double();
            if (timeout_f < 0)
                env->raise("ArgumentError", "time interval must not be negative");
            timeout = timeout_f * 1000;
        }
        for (;;) {
            result = poll(&fds, 1, timeout);
            if (result == -1 && errno == EINTR) {
                // Interrupted by a signal -- probably the GC stopping the world.
                // Try again.
            } else if (result == -1) {
                env->raise_errno();
            } else if (result == 0) {
                auto TimeoutError = fetch_nested_const({ "IO"_s, "TimeoutError"_s });
                auto error = Object::_new(env, TimeoutError, { StringObject::create("Connect timed out!") }, nullptr).as_exception();
                env->raise_exception(error);
            } else {
                // poll() can return a positive value even if the socket is not writable.
                // We need to check the socket options to see if it really is writable.
                int option_value = 0;
                socklen_t option_len = sizeof(option_value);
                if (getsockopt(self.as_io()->fileno(), SOL_SOCKET, SO_ERROR, &option_value, &option_len) == -1)
                    env->raise_errno();
                if (option_value)
                    env->raise_errno(option_value);
                break;
            }
        }
    }

    return Value::integer(0);
}

Value Socket_listen(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 1);
    auto backlog = args.at(0).integer_or_raise(env).to_nat_int_t();

    auto result = listen(self.as_io()->fileno(), backlog);
    if (result == -1)
        env->raise_errno();

    return Value::integer(result);
}

Value Socket_recvfrom(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 1, 2);
    const auto maxlen = IntegerMethods::convert_to_native_type<size_t>(env, args.at(0));
    auto flags = 0;
    if (!args.at(1, Value::nil()).is_nil())
        flags = IntegerMethods::convert_to_native_type<int>(env, args.at(1));

    String str { maxlen, '\0' };
    sockaddr_storage src_addr;
    memset(&src_addr, 0, sizeof(src_addr));
    socklen_t addrlen = sizeof(src_addr);

    // Set src_addr.ss_family value
    if (getsockname(self.as_io()->fileno(), reinterpret_cast<sockaddr *>(&src_addr), &addrlen) < 0)
        env->raise_errno();
    const auto family = src_addr.ss_family;
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.ss_family = family;
    addrlen = sizeof(src_addr);

    const auto res = recvfrom(
        self.as_io()->fileno(),
        &str[0], maxlen,
        flags,
        reinterpret_cast<sockaddr *>(&src_addr), &addrlen);
    if (res < 0)
        env->raise_errno();
    auto Addrinfo = find_top_level_const(env, "Addrinfo"_s);
    auto addrinfo = StringObject::create(reinterpret_cast<const char *>(&src_addr), addrlen, Encoding::ASCII_8BIT);
    int socktype = 0;
    socklen_t socktype_len = sizeof(socktype);
    if (getsockopt(self.as_io()->fileno(), SOL_SOCKET, SO_TYPE, &socktype, &socktype_len) == -1)
        env->raise_errno();
    str.truncate(res);
    return ArrayObject::create({
        StringObject::create(std::move(str), Encoding::ASCII_8BIT),
        Addrinfo.send(env, "new"_s, { addrinfo, Value::integer(family), Value::integer(socktype) }),
    });
}

Value Socket_sysaccept(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 0);
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    auto socket = Server_sysaccept(env, self, addr, len, true);
    return Socket_accept(env, socket, addr, len);
}

Value Socket_pair(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_between(env, 2, 3);
    const auto domain = Socket_const_get(env, args.at(0));
    const auto type = Socket_const_get(env, args.at(1));
    const auto protocol = Socket_const_get(env, args.at(2, Value::nil()), true);

    int fd[2];
    if (socketpair(domain, type, protocol, fd) < 0)
        env->raise_errno();

    return ArrayObject::create({ self.send(env, "for_fd"_s, { Value::integer(fd[0]) }),
        self.send(env, "for_fd"_s, { Value::integer(fd[1]) }) });
}

Value Socket_pack_sockaddr_in(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 2);
    auto service = args.at(0);
    auto host = args.at(1);
    if (host.is_nil())
        host = StringObject::create("127.0.0.1");
    if (host.is_integer() && host.integer().is_fixnum() && host.integer().to_nat_int_t() == INADDR_ANY)
        host = StringObject::create("0.0.0.0");
    if (host.is_string() && host.as_string()->is_empty())
        host = StringObject::create("0.0.0.0");

    struct addrinfo hints { };
    hints.ai_family = PF_UNSPEC;

    String service_str;
    if (service.is_nil())
        service_str = "";
    else if (service.is_string())
        service_str = service.as_string()->string();
    else if (service.is_integer())
        service_str = service.integer().to_string();
    else
        service_str = service.to_str(env)->string();

    struct addrinfo *addr = nullptr;
    auto result = getaddrinfo(host.as_string_or_raise(env)->c_str(), service_str.c_str(), &hints, &addr);

    if (result != 0)
        env->raise("SocketError", "getaddrinfo: {}", gai_strerror(result));

    Defer freeinfo { [&addr] { freeaddrinfo(addr); } };

    auto packed = StringObject::create(reinterpret_cast<const char *>(addr->ai_addr), addr->ai_addrlen, Encoding::ASCII_8BIT);

    return packed;
}

Value Socket_pack_sockaddr_un(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 1);
    auto path = args.at(0);
    path.assert_type(env, Object::Type::String, "String");
    auto path_string = path.as_string();

    struct sockaddr_un un { };
    constexpr auto unix_path_max = sizeof(un.sun_path);

    if (path_string->length() >= unix_path_max)
        env->raise("ArgumentError", "too long unix socket path ({} bytes given but {} bytes max))", path_string->length(), unix_path_max);

    un.sun_family = AF_UNIX;
    memcpy(un.sun_path, path_string->c_str(), path_string->length());
    return StringObject::create((const char *)&un, sizeof(un), Encoding::ASCII_8BIT);
}

Value Socket_unpack_sockaddr_in(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 1);
    auto sockaddr = args.at(0);

    if (sockaddr.is_a(env, find_top_level_const(env, "Addrinfo"_s))) {
        auto afamily = sockaddr.send(env, "afamily"_s).send(env, "to_i"_s).integer().to_nat_int_t();
        if (afamily != AF_INET && afamily != AF_INET6)
            env->raise("ArgumentError", "not an AF_INET/AF_INET6 sockaddr");
        auto host = sockaddr.send(env, "ip_address"_s);
        auto port = sockaddr.send(env, "ip_port"_s);
        return ArrayObject::create({ port, host });
    }

    sockaddr.assert_type(env, Object::Type::String, "String");

    auto family = reinterpret_cast<const struct sockaddr *>(sockaddr.as_string()->c_str())->sa_family;

    String sockaddr_string = sockaddr.as_string()->string();
    Value port;
    Value host;

    switch (family) {
    case AF_INET: {
        auto in = reinterpret_cast<const sockaddr_in *>(sockaddr_string.c_str());
        char host_buf[INET_ADDRSTRLEN];
        auto result = inet_ntop(AF_INET, &in->sin_addr, host_buf, INET_ADDRSTRLEN);
        if (!result)
            env->raise_errno();
        port = Value::integer(ntohs(in->sin_port));
        host = StringObject::create(host_buf);
        break;
    }
    case AF_INET6: {
        auto in = reinterpret_cast<const sockaddr_in6 *>(sockaddr_string.c_str());
        char host_buf[INET6_ADDRSTRLEN];
        auto result = inet_ntop(AF_INET6, &in->sin6_addr, host_buf, INET6_ADDRSTRLEN);
        if (!result)
            env->raise_errno();
        auto ary = ArrayObject::create();
        port = Value::integer(ntohs(in->sin6_port));
        host = StringObject::create(host_buf);
        break;
    }
    default:
        env->raise("ArgumentError", "not an AF_INET/AF_INET6 sockaddr");
    }

    auto ary = ArrayObject::create();
    ary->push(port);
    ary->push(host);
    return ary;
}

Value Socket_unpack_sockaddr_un(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 1);
    auto sockaddr = args.at(0);

    if (sockaddr.is_a(env, find_top_level_const(env, "Addrinfo"_s))) {
        auto afamily = sockaddr.send(env, "afamily"_s).send(env, "to_i"_s).integer().to_nat_int_t();
        if (afamily != AF_UNIX)
            env->raise("ArgumentError", "not an AF_UNIX sockaddr");
        return sockaddr.send(env, "unix_path"_s);
    }

    sockaddr.assert_type(env, Object::Type::String, "String");

    if (sockaddr.as_string()->bytesize() > sizeof(struct sockaddr_un))
        env->raise("ArgumentError", "not an AF_UNIX sockaddr");

    struct sockaddr_un addr { };
    memcpy(&addr, sockaddr.as_string()->c_str(), std::min(sizeof(addr), sockaddr.as_string()->bytesize()));
    if (addr.sun_family != AF_UNIX)
        env->raise("ArgumentError", "not an AF_UNIX sockaddr");
    // NATFIXME: Change to ASCII_8BIT, but this currently breaks the specs due to a missing String#encode with 2 arguments
    return StringObject::create(addr.sun_path);
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
        auto sockaddr = reinterpret_cast<sockaddr_in *>(result->ai_addr);
        return ntohs(sockaddr->sin_port);
    }
    case AF_INET6: {
        auto sockaddr = reinterpret_cast<sockaddr_in6 *>(result->ai_addr);
        return ntohs(sockaddr->sin6_port);
    }
    default:
        NAT_NOT_YET_IMPLEMENTED("port for getaddrinfo result %d", result->ai_family);
    }
}

static String Socket_getaddrinfo_result_host(struct addrinfo *result) {
    switch (result->ai_family) {
    case AF_INET: {
        auto sockaddr = reinterpret_cast<sockaddr_in *>(result->ai_addr);
        char address[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sockaddr->sin_addr), address, INET_ADDRSTRLEN);
        return address;
    }
    case AF_INET6: {
        auto sockaddr = reinterpret_cast<sockaddr_in6 *>(result->ai_addr);
        char address[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(sockaddr->sin6_addr), address, INET6_ADDRSTRLEN);
        return address;
    }
    default:
        NAT_NOT_YET_IMPLEMENTED("port for getaddrinfo result %d", result->ai_family);
    }
}

Value Socket_s_getaddrinfo(Env *env, Value self, Args &&args, Block *) {
    // getaddrinfo(nodename, servname[, family[, socktype[, protocol[, flags[, reverse_lookup]]]]]) => array
    args.ensure_argc_between(env, 2, 7);
    auto nodename = args.at(0);
    auto servname = args.at(1);
    auto family = args.at(2, Value::nil());
    auto socktype = args.at(3, Value::nil());
    auto protocol = args.at(4, Value::nil());
    auto flags = args.at(5, Value::nil());
    auto reverse_lookup = args.at(6, Value::nil());

    if (reverse_lookup.is_nil()) {
        auto BasicSocket = find_top_level_const(env, "BasicSocket"_s);
        reverse_lookup = BasicSocket.send(env, "do_not_reverse_lookup"_s).send(env, "!"_s);
    } else if (reverse_lookup == "numeric"_s) {
        reverse_lookup = Value::False();
    }

    struct addrinfo hints { };
    hints.ai_family = Socket_const_name_to_i(env, self, { family, Value::True() }, nullptr).integer_or_raise(env).to_nat_int_t();
    hints.ai_socktype = Socket_const_name_to_i(env, self, { socktype, Value::True() }, nullptr).integer_or_raise(env).to_nat_int_t();
    hints.ai_protocol = Socket_const_name_to_i(env, self, { protocol, Value::True() }, nullptr).integer_or_raise(env).to_nat_int_t();
    hints.ai_flags = Socket_const_name_to_i(env, self, { flags, Value::True() }, nullptr).integer_or_raise(env).to_nat_int_t();

    String host;
    String service;

    if (nodename.is_nil() || (nodename.is_string() && nodename.as_string()->is_empty()))
        host = "";
    else if (nodename.is_string())
        host = nodename.as_string_or_raise(env)->string();
    else if (nodename.respond_to(env, "to_str"_s))
        host = nodename.to_str(env)->string();

    if (servname.is_nil() || (servname.is_string() && servname.as_string()->is_empty()))
        service = "0";
    else if (servname.is_integer())
        service = servname.integer().to_string();
    else
        service = servname.as_string_or_raise(env)->string();

    struct addrinfo *result = nullptr;
    int s = getaddrinfo(
        host.is_empty() ? nullptr : host.c_str(),
        service.c_str(),
        &hints,
        &result);

    if (s != 0)
        env->raise("SocketError", "getaddrinfo: {}", gai_strerror(s));

    Defer freeinfo([&result] { freeaddrinfo(result); });

    auto ary = ArrayObject::create();

    struct addrinfo *info = result;
    do {
        auto addr = ArrayObject::create();
        addr->push(StringObject::create(Socket_family_to_string(info->ai_family)));
        addr->push(Value::integer(Socket_getaddrinfo_result_port(info)));
        if (reverse_lookup.is_truthy())
            addr->push(StringObject::create(Socket_reverse_lookup_address(env, info->ai_addr)));
        else
            addr->push(StringObject::create(Socket_getaddrinfo_result_host(info)));
        addr->push(StringObject::create(Socket_getaddrinfo_result_host(info)));
        addr->push(Value::integer(info->ai_family));
        addr->push(Value::integer(info->ai_socktype));
        addr->push(Value::integer(info->ai_protocol));
        ary->push(addr);
        info = info->ai_next;
    } while (info);

    return ary;
}

Value Socket_s_gethostname(Env *env, Value, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) < 0)
        env->raise_errno();
    return StringObject::create(hostname, Encoding::ASCII_8BIT);
}

Value Socket_s_getifaddrs(Env *env, Value, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);
    ifaddrs *ifa;
    if (getifaddrs(&ifa) < 0)
        env->raise_errno();
    Defer freeifa { [&ifa] { freeifaddrs(ifa); } };

    auto Ifaddr = fetch_nested_const({ "Socket"_s, "Ifaddr"_s });
    auto Addrinfo = find_top_level_const(env, "Addrinfo"_s).as_class();
    auto sockaddr_to_addrinfo = [&env, &Addrinfo](sockaddr *addr) -> Value {
        if (!addr)
            return Value::nil();
        size_t len = 0;
        switch (addr->sa_family) {
        case AF_INET:
            len = sizeof(sockaddr_in);
            break;
        case AF_INET6:
            len = sizeof(sockaddr_in6);
            break;
#ifdef AF_PACKET
        case AF_PACKET: // NATFIXME: Support AF_PACKET (Ethernet) addr, see packet(7)
#endif
        default:
            return Value::nil();
        }
        auto addrinfo_str = StringObject::create(reinterpret_cast<const char *>(addr), len);
        return Object::_new(env, Addrinfo, { addrinfo_str }, nullptr);
    };
    auto result = ArrayObject::create();
    for (ifaddrs *it = ifa; it != nullptr; it = it->ifa_next) {
        auto ifaddr = Object::allocate(env, Ifaddr, {}, nullptr);
        auto name = StringObject::create(it->ifa_name);
        ifaddr->ivar_set(env, "@name"_s, name);
        const auto ifindex = if_nametoindex(it->ifa_name);
        ifaddr->ivar_set(env, "@ifindex"_s, Value::integer(ifindex));
        ifaddr->ivar_set(env, "@flags"_s, Value::integer(it->ifa_flags));
        ifaddr->ivar_set(env, "@addr"_s, sockaddr_to_addrinfo(it->ifa_addr));
        ifaddr->ivar_set(env, "@netmask"_s, sockaddr_to_addrinfo(it->ifa_netmask));
#ifdef __APPLE__
        if (it->ifa_flags & IFF_BROADCAST)
            ifaddr->ivar_set(env, "@broadaddr"_s, sockaddr_to_addrinfo(it->ifa_dstaddr));
        if (it->ifa_flags & IFF_POINTOPOINT)
            ifaddr->ivar_set(env, "@dstaddr"_s, sockaddr_to_addrinfo(it->ifa_dstaddr));
#else
        if (it->ifa_flags & IFF_BROADCAST)
            ifaddr->ivar_set(env, "@broadaddr"_s, sockaddr_to_addrinfo(it->ifa_ifu.ifu_broadaddr));
        if (it->ifa_flags & IFF_POINTOPOINT)
            ifaddr->ivar_set(env, "@dstaddr"_s, sockaddr_to_addrinfo(it->ifa_ifu.ifu_dstaddr));
#endif
        result->push(ifaddr);
    }
    return result;
}

Value Socket_s_getservbyname(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 1, 2);
    auto name = args[0].to_str(env);
    const char *proto = "tcp";
    if (auto proto_val = args.at(1, Value::nil()); !proto_val.is_nil())
        proto = proto_val.to_str(env)->c_str();

    auto result = getservbyname(name->c_str(), proto);
    if (!result)
        env->raise("SocketError", "no such service {}/{}", name->string(), proto);
    return Value::integer(ntohs(result->s_port));
}

Value Socket_s_getservbyport(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_between(env, 1, 2);
    auto port = IntegerMethods::convert_to_native_type<int>(env, args[0]);
    const char *proto = "tcp";
    if (auto proto_val = args.at(1, Value::nil()); !proto_val.is_nil())
        proto = proto_val.to_str(env)->c_str();

    auto result = getservbyport(port, proto);
    if (!result)
        env->raise("SocketError", "no such service for port {}/{}", port, proto);
    return StringObject::create(result->s_name);
}

Value Socket_Option_bool(Env *env, Value self, Args &&, Block *) {
    auto data = self->ivar_get(env, "@data"_s).as_string_or_raise(env);

    switch (data->length()) {
    case 1:
        return bool_object(data->c_str()[0] != 0);
    case sizeof(int): {
        auto i = *(int *)data->c_str();
        return bool_object(i != 0);
    }
    default:
        return Value::False();
    }
}

Value Socket_Option_int(Env *env, Value self, Args &&, Block *) {
    auto data = self->ivar_get(env, "@data"_s).as_string_or_raise(env);

    if (data->length() != sizeof(int))
        return Value::integer(0);

    auto i = *(int *)data->c_str();
    return Value::integer(i);
}

Value Socket_Option_s_linger(Env *env, Value self, Args &&args, Block *) {
    unsigned short family = 0; // any
    unsigned short level = SOL_SOCKET;
    unsigned short optname = SO_LINGER;

    args.ensure_argc_is(env, 2);
    auto on_off = args.at(0).is_truthy();
    int linger = args.at(1).to_int(env).to_nat_int_t();

    struct linger data {
        on_off, linger
    };

    auto data_string = StringObject::create((const char *)(&data), sizeof(data));

    return self.send(env, "new"_s, { Value::integer(family), Value::integer(level), Value::integer(optname), data_string });
}

Value Socket_Option_linger(Env *env, Value self, Args &&, Block *) {
    auto data = self->ivar_get(env, "@data"_s).as_string_or_raise(env);

    if (data->length() != sizeof(struct linger))
        return Value::integer(0);

    auto l = (struct linger *)data->c_str();
    auto on_off = bool_object(l->l_onoff != 0);
    auto linger = Value::integer(l->l_linger);
    return ArrayObject::create({ on_off, linger });
}

Value TCPSocket_initialize(Env *env, Value self, Args &&args, Block *block) {
    auto kwargs = args.pop_keyword_hash();
    args.ensure_argc_between(env, 2, 4);

    auto host = args.at(0);
    auto port = args.at(1);
    auto local_host = args.at(2, Value::nil());
    auto local_port = args.at(3, Value::nil());
    auto connect_timeout = kwargs ? kwargs->remove(env, "connect_timeout"_s) : Optional<Value> {};
    env->ensure_no_extra_keywords(kwargs);

    auto domain = AF_INET;
    if (host.is_string() && !host.as_string()->is_empty()) {
        struct addrinfo *info = nullptr;
        const auto result = getaddrinfo(host.as_string()->c_str(), nullptr, nullptr, &info);
        if (result != 0) {
            if (result == EAI_SYSTEM)
                env->raise_errno();
            env->raise("SocketError", "getaddrinfo: {}", gai_strerror(result));
        }
        Defer freeinfo { [&info] { freeaddrinfo(info); } };
        domain = info->ai_family;
    }

    auto fd = socket(domain, SOCK_STREAM, 0);
    if (fd == -1)
        env->raise_errno();

    self.as_io()->initialize(env, { Value::integer(fd) }, block);
    self.as_io()->binmode(env);
    self.as_io()->set_close_on_exec(env, Value::True());

    auto Socket = find_top_level_const(env, "Socket"_s);

    if (local_host.is_truthy() || local_port.is_truthy()) {
        auto local_sockaddr = Socket.send(env, "pack_sockaddr_in"_s, { local_port, local_host });
        Socket_bind(env, self, { local_sockaddr }, nullptr);
    }

    auto sockaddr = Socket.send(env, "pack_sockaddr_in"_s, { port, host });
    self.as_io()->set_nonblock(env, true);
    Args new_args = { sockaddr };
    if (connect_timeout) {
        auto new_kwargs = HashObject::create(env, { "connect_timeout"_s, *connect_timeout });
        Socket_connect(env, self, Args({ sockaddr, new_kwargs }, true), nullptr);
    } else {
        Socket_connect(env, self, { sockaddr }, nullptr);
    }
    self->ivar_set(env, "@do_not_reverse_lookup"_s, find_top_level_const(env, "BasicSocket"_s).send(env, "do_not_reverse_lookup"_s));

    return self;
}

Value TCPServer_initialize(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_between(env, 1, 2);
    auto hostname = args.at(0);
    auto port = args.at(1, Value::nil());

    // TCPServer.new([hostname,] port)
    if (args.size() == 1) {
        port = hostname;
        hostname = StringObject::create("0.0.0.0");
    }

    auto domain = AF_INET;
    if (hostname.is_string() && !hostname.as_string()->is_empty()) {
        struct addrinfo *info = nullptr;
        const auto result = getaddrinfo(hostname.as_string()->c_str(), nullptr, nullptr, &info);
        if (result != 0) {
            if (result == EAI_SYSTEM)
                env->raise_errno();
            env->raise("SocketError", "getaddrinfo: {}", gai_strerror(result));
        }
        Defer freeinfo { [&info] { freeaddrinfo(info); } };
        domain = info->ai_family;
    }

    auto fd = socket(domain, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1)
        env->raise_errno();

    self.as_io()->initialize(env, { Value::integer(fd) }, block);
    self.as_io()->binmode(env);
    self.as_io()->set_nonblock(env, true);
    self->ivar_set(env, "@do_not_reverse_lookup"_s, find_top_level_const(env, "BasicSocket"_s).send(env, "do_not_reverse_lookup"_s));

    self.send(env, "setsockopt"_s, { "SOCKET"_s, "REUSEADDR"_s, Value::True() });

    auto Socket = find_top_level_const(env, "Socket"_s);
    auto sockaddr = Socket.send(env, "pack_sockaddr_in"_s, { port, hostname });
    Socket_bind(env, self, { sockaddr }, nullptr);
    Socket_listen(env, self, { Value::integer(1) }, nullptr);

    return self;
}

Value TCPServer_accept(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    if (self.as_io()->is_closed())
        env->raise("IOError", "closed stream");

    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    return Server_accept(env, self, "TCPSocket"_s, addr, len, true);
}

Value TCPServer_accept_nonblock(Env *env, Value self, Args &&args, Block *) {
    auto kwargs = args.pop_keyword_hash();
    auto exception = kwargs ? kwargs->remove(env, "exception"_s) : Value::True();
    args.ensure_argc_is(env, 0);
    env->ensure_no_extra_keywords(kwargs);

    if (self.as_io()->is_closed())
        env->raise("IOError", "closed stream");

    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    return Server_accept(env, self, "TCPSocket"_s, addr, len, false, exception.value().is_truthy());
}

Value TCPServer_listen(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 1);
    return Socket_listen(env, self, std::move(args), nullptr);
}

Value TCPServer_sysaccept(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 0);
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    return Server_sysaccept(env, self, addr, len, true);
}

Value UDPSocket_initialize(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_between(env, 0, 1);
    auto family = Socket_const_name_to_i(env, self, { args.at(0, Value::integer(AF_INET)) }, nullptr);

    auto fd = socket(family.integer_or_raise(env).to_nat_int_t(), SOCK_DGRAM, 0);
    if (fd == -1)
        env->raise_errno();

    self.as_io()->initialize(env, { Value::integer(fd) }, block);
    self.as_io()->binmode(env);
    self.as_io()->set_close_on_exec(env, Value::True());
    self.as_io()->set_nonblock(env, true);
    self->ivar_set(env, "@do_not_reverse_lookup"_s, find_top_level_const(env, "BasicSocket"_s).send(env, "do_not_reverse_lookup"_s));

    return self;
}

Value UDPSocket_bind(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 2);

    auto Socket = find_top_level_const(env, "Socket"_s);
    auto sockaddr = Socket.send(env, "sockaddr_in"_s, { args.at(1), args.at(0) }, nullptr);
    return Socket_bind(env, self, { sockaddr }, nullptr);
}

Value UDPSocket_connect(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 2);

    auto Socket = find_top_level_const(env, "Socket"_s);
    auto sockaddr = Socket.send(env, "sockaddr_in"_s, { args.at(1), args.at(0) }, nullptr);
    return Socket_connect(env, self, { sockaddr }, nullptr);
}

Value UDPSocket_recvfrom_nonblock(Env *env, Value self, Args &&args, Block *) {
    auto kwargs = args.pop_keyword_hash();
    auto exception = kwargs ? kwargs->remove(env, "exception"_s) : Value::True();
    args.ensure_argc_between(env, 1, 3);
    env->ensure_no_extra_keywords(kwargs);

    const auto maxlen = IntegerMethods::convert_to_nat_int_t(env, args[0]);
    auto flags = IntegerMethods::convert_to_nat_int_t(env, args.at(1, Value::integer(0)));
    auto buffer = args.at(2, StringObject::create("", Encoding::ASCII_8BIT)).to_str(env);
    char buf[maxlen];
    sockaddr_storage addr {};
    socklen_t addr_len = sizeof(addr);

    const auto recvfrom_result = recvfrom(
        self.as_io()->fileno(),
        buf, maxlen,
        flags | MSG_DONTWAIT,
        reinterpret_cast<struct sockaddr *>(&addr), &addr_len);
    if (recvfrom_result < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            if (exception.value().is_falsey())
                return "wait_readable"_s;
            auto SystemCallError = find_top_level_const(env, "SystemCallError"_s);
            ExceptionObject *error = SystemCallError.send(env, "exception"_s, { Value::integer(errno) }).as_exception();
            auto WaitReadable = fetch_nested_const({ "IO"_s, "WaitReadable"_s }).as_module_or_raise(env);
            error->extend_once(env, WaitReadable);
            env->raise_exception(error);
        } else {
            env->raise_errno();
        }
    } else if (recvfrom_result == 0) {
        return Value::nil();
    }

    Value sender_inet_addr;
    switch (addr.ss_family) {
    case AF_INET: {
        sockaddr_in *addr_in = reinterpret_cast<sockaddr_in *>(&addr);
        char host_buf[INET_ADDRSTRLEN];
        auto ntop_result = inet_ntop(AF_INET, &addr_in->sin_addr, host_buf, INET_ADDRSTRLEN);
        if (!ntop_result)
            env->raise_errno();
        auto ip = StringObject::create(host_buf);
        sender_inet_addr = ArrayObject::create({ StringObject::create("AF_INET"),
            Value::integer(ntohs(addr_in->sin_port)),
            ip,
            ip });
        break;
    }
    case AF_INET6: {
        sockaddr_in6 *addr_in6 = reinterpret_cast<sockaddr_in6 *>(&addr);
        char host_buf[INET6_ADDRSTRLEN];
        auto ntop_result = inet_ntop(AF_INET, &addr_in6->sin6_addr, host_buf, INET6_ADDRSTRLEN);
        if (!ntop_result)
            env->raise_errno();
        auto ip = StringObject::create(host_buf);
        sender_inet_addr = ArrayObject::create({ StringObject::create("AF_INET6"),
            Value::integer(ntohs(addr_in6->sin6_port)),
            ip,
            ip });
        break;
    }
    default:
        NAT_NOT_YET_IMPLEMENTED("UDPSocket#recvfrom_nonblock for family %d", addr.ss_family);
    }
    buffer->set_str(buf, recvfrom_result);
    return ArrayObject::create({ buffer,
        sender_inet_addr });
}

Value UNIXSocket_initialize(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 1);

    auto path = args.at(0).to_str(env);
    if (*path->c_str() != 0 && strlen(path->c_str()) != path->bytesize())
        env->raise("ArgumentError", "path name contains null byte");

    auto fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1)
        env->raise_errno();

    self.as_io()->initialize(env, { Value::integer(fd) }, block);
    self.as_io()->binmode(env);
    self.as_io()->set_close_on_exec(env, Value::True());
    self->ivar_set(env, "@do_not_reverse_lookup"_s, find_top_level_const(env, "BasicSocket"_s).send(env, "do_not_reverse_lookup"_s));

    auto Socket = find_top_level_const(env, "Socket"_s);
    auto sockaddr = Socket.send(env, "pack_sockaddr_un"_s, { path });
    Socket_connect(env, self, { sockaddr }, nullptr);
    self.as_io()->set_nonblock(env, true);

    if (block) {
        try {
            block->run(env, { self }, nullptr);
            Socket_close(env, self, {}, nullptr);
        } catch (ExceptionObject *exception) {
            Socket_close(env, self, {}, nullptr);
        }
    }

    return self;
}

Value UNIXServer_initialize(Env *env, Value self, Args &&args, Block *block) {
    args.ensure_argc_is(env, 1);

    auto path = args.at(0).to_str(env);
    if (*path->c_str() != 0 && strlen(path->c_str()) != path->bytesize())
        env->raise("ArgumentError", "path name contains null byte");

    auto fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd == -1)
        env->raise_errno();

    auto kwargs = HashObject::create(env, { "path"_s, path });
    self.as_io()->initialize(env, Args({ Value::integer(fd), kwargs }, true), block);
    self.as_io()->binmode(env);
    self->ivar_set(env, "@do_not_reverse_lookup"_s, find_top_level_const(env, "BasicSocket"_s).send(env, "do_not_reverse_lookup"_s));

    auto Socket = find_top_level_const(env, "Socket"_s);
    auto sockaddr = Socket.send(env, "pack_sockaddr_un"_s, { path });
    Socket_bind(env, self, { sockaddr }, nullptr);
    Socket_listen(env, self, { Value::integer(1) }, nullptr);

    return self;
}

Value UNIXServer_accept(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    return Server_accept(env, self, "UNIXSocket"_s, addr, len, true);
}

Value UNIXServer_accept_nonblock(Env *env, Value self, Args &&args, Block *) {
    auto kwargs = args.pop_keyword_hash();
    auto exception = kwargs ? kwargs->remove(env, "exception"_s) : Value::True();
    args.ensure_argc_is(env, 0);
    env->ensure_no_extra_keywords(kwargs);
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    return Server_accept(env, self, "UNIXSocket"_s, addr, len, false, exception.value().is_truthy());
}

Value UNIXServer_listen(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 1);
    return Socket_listen(env, self, std::move(args), nullptr);
}

Value UNIXServer_sysaccept(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    return Server_sysaccept(env, self, addr, len, true);
}
