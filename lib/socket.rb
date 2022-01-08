require 'natalie/inline'

__inline__ <<-END
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
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
  SOCK_STREAM = __constant__('SOCK_STREAM', 'unsigned short')
  SOL_SOCKET = __constant__('SOL_SOCKET', 'unsigned short')
  SO_REUSEADDR = __constant__('SO_REUSEADDR', 'unsigned short')
  SOCK_DGRAM = __constant__('SOCK_DGRAM', 'unsigned short')

  def initialize(domain, socktype, protocol = nil)
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
    __define_method__ :sockaddr_in, [:port, :host], <<-END
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

    __define_method__ :unpack_sockaddr_in, [:sockaddr], <<-END
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
  end

  def self.pack_sockaddr_in(port, host)
    # TODO
  end
end

class Addrinfo
  def self.tcp(ip, port)
    Addrinfo.new # TODO
  end

  def self.udp(ip, port)
    Addrinfo.new # TODO
  end

  def afamily
    # TODO
  end

  def socktype
    # TODO
  end
end
