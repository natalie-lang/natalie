require 'natalie/inline'

__inline__ <<-END
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
END

class SocketError < StandardError
end

class BasicSocket
  def setsockopt(level, optname, optval)
    # TODO
  end
end

class Socket < BasicSocket
  AF_INET = __constant__('AF_INET', 'unsigned short')
  AF_INET6 = __constant__('AF_INET6', 'unsigned short')
  AF_UNIX = __constant__('AF_UNIX', 'unsigned short')
  IPPROTO_TCP = __constant__('IPPROTO_TCP', 'unsigned short')
  PF_INET = __constant__('PF_INET', 'unsigned short')
  PF_INET6 = __constant__('PF_INET6', 'unsigned short')
  PF_UNSPEC = __constant__('PF_UNSPEC', 'unsigned short')
  SOCK_RAW = __constant__('SOCK_RAW', 'unsigned short')
  SOCK_RDM = __constant__('SOCK_RDM', 'unsigned short')
  SOCK_STREAM = __constant__('SOCK_STREAM', 'unsigned short')
  SOL_SOCKET = __constant__('SOL_SOCKET', 'unsigned short')
  SO_REUSEADDR = __constant__('SO_REUSEADDR', 'unsigned short')
  SOCK_DGRAM = __constant__('SOCK_DGRAM', 'unsigned short')
  SOCK_SEQPACKET = __constant__('SOCK_SEQPACKET', 'unsigned short')

  def initialize(domain, socktype, protocol = nil)
    super()
    @domain = domain
    @socktype = socktype
    @protocol = protocol
  end

  def bind(address)
    # TODO
  end

  def local_address
    # TODO
    Addrinfo.new
  end

  def close
    # TODO
  end

  def closed?
    false # TODO
  end

  class << self
    __define_method__ :pack_sockaddr_in, %i[port host], <<-END
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
        struct sockaddr_in6 in {};
#if defined(__OpenBSD__) or defined(__APPLE__)
        in.sin6_len = sizeof(in);
#endif
        in.sin6_family = AF_INET6;
        in.sin6_port = port_unsigned;
        memcpy(in.sin6_addr.s6_addr, host_string->c_str(), std::min(host_string->length(), (size_t)16));
        return new StringObject { (const char*)&in, sizeof(in) };

      } else {

        // IPV4
        struct in_addr a;
        auto result = inet_aton(host->as_string()->c_str(), &a);
        if (!result)
            env->raise("SocketError", "getaddrinfo: Name or service not known");
        struct sockaddr_in in {};
#if defined(__OpenBSD__) or defined(__APPLE__)
        in.sin_len = sizeof(in);
#endif
        in.sin_family = AF_INET;
        in.sin_port = port_unsigned;
        in.sin_addr = a;
        return new StringObject { (const char*)&in, sizeof(in) };

      }
    END

    alias sockaddr_in pack_sockaddr_in

    __define_method__ :pack_sockaddr_un, [:path], <<-END
      path->assert_type(env, Object::Type::String, "String");
      auto path_string = path->as_string();

      struct sockaddr_un un {};
      constexpr auto unix_path_max = sizeof(un.sun_path);

      if (path_string->length() >= unix_path_max)
          env->raise("ArgumentError", "too long unix socket path ({} bytes given but {} bytes max))", path_string->length(), unix_path_max);

      un.sun_family = AF_UNIX;
      memcpy(un.sun_path, path_string->c_str(), path_string->length());
      return new StringObject { (const char*)&un, sizeof(un) };
    END

    alias sockaddr_un pack_sockaddr_un

    __define_method__ :unpack_sockaddr_in, [:sockaddr], <<-END
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
    END

    __define_method__ :unpack_sockaddr_un, [:sockaddr], <<-END
      if (sockaddr->is_a(env, self->const_find(env, "Addrinfo"_s, Object::ConstLookupSearchMode::NotStrict)))
          sockaddr = sockaddr->send(env, "sockaddr"_s);

      sockaddr->assert_type(env, Object::Type::String, "String");

      if (sockaddr->as_string()->length() != sizeof(struct sockaddr_un))
          env->raise("ArgumentError", "not an AF_UNIX sockaddr");

      const char *str = sockaddr->as_string()->c_str();
      auto un = (struct sockaddr_un *)str;
      return new StringObject { un->sun_path };
    END
  end
end

class Addrinfo
  __define_method__ :initialize, <<-END
    args.ensure_argc_between(env, 1, 4);
    auto sockaddr = args.at(0);
    auto family = args.at(1, NilObject::the());
    auto socktype = args.at(2, NilObject::the());
    auto protocol = args.at(3, NilObject::the());

    self->ivar_set(env, "@sockaddr"_s, sockaddr);

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

    struct addrinfo hints {};
    struct addrinfo *result {};
    if (!family->is_nil())
        hints.ai_family = (unsigned short)family->as_integer()->to_nat_int_t();
    if (!socktype->is_nil())
        hints.ai_socktype = (unsigned short)socktype->as_integer()->to_nat_int_t();
    hints.ai_flags = 0;
    //hints.ai_protocol = IPPROTO_UDP;

    struct sockaddr_un un {};
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
            &result
        );
        if (s != 0)
            env->raise("SocketError", "getaddrinfo: {}\\n", gai_strerror(s));
        self->ivar_set(env, "@_addrinfo"_s, new VoidPObject { result });
    }

    return self;
  END

  attr_reader :sockaddr, :family

  def self.tcp(ip, port)
    Addrinfo.new(Socket.pack_sockaddr_in(port, ip))
  end

  def self.udp(_ip, _port)
    Addrinfo.new # TODO
  end

  def self.unix(path)
    Addrinfo.new(Socket.pack_sockaddr_un(path))
  end

  def afamily
    # TODO
  end

  __define_method__ :socktype, <<-END
      auto addrinfo_void_ptr = self->ivar_get(env, "@_addrinfo"_s)->as_void_p();
      auto info_struct = (struct addrinfo *)addrinfo_void_ptr->void_ptr();
      return Value::integer(info_struct->ai_socktype);
  END

  def unix_path
    # TODO
  end

  __define_method__ :pfamily, <<-END
      auto addrinfo_void_ptr = self->ivar_get(env, "@_addrinfo"_s)->as_void_p();
      auto info_struct = (struct addrinfo *)addrinfo_void_ptr->void_ptr();
      return Value::integer(info_struct->ai_family);
  END

  def protocol
    # TODO
  end

  def ip_address
    # TODO
  end

  def ip_port
    # TODO
  end
end
