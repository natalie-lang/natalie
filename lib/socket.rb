require 'natalie/inline'
require 'socket.cpp'

class SocketError < StandardError
end

class BasicSocket < IO
  __bind_method__ :getpeername, :BasicSocket_getpeername
  __bind_method__ :getsockname, :BasicSocket_getsockname
  __bind_method__ :getsockopt, :BasicSocket_getsockopt
  __bind_method__ :local_address, :BasicSocket_local_address
  __bind_method__ :recv, :BasicSocket_recv
  __bind_method__ :recv_nonblock, :BasicSocket_recv_nonblock
  __bind_method__ :send, :BasicSocket_send
  __bind_method__ :setsockopt, :BasicSocket_setsockopt
  __bind_method__ :shutdown, :BasicSocket_shutdown

  attr_writer :do_not_reverse_lookup

  def do_not_reverse_lookup
    if @do_not_reverse_lookup.nil?
      self.class.do_not_reverse_lookup
    else
      @do_not_reverse_lookup
    end
  end

  def read_nonblock(maxlen, *args, **kwargs)
    recv_nonblock(maxlen, 0, *args, **kwargs)
  end

  class << self
    __bind_method__ :for_fd, :BasicSocket_s_for_fd

    def do_not_reverse_lookup
      case @do_not_reverse_lookup
      when nil
        true
      when false
        false
      else
        true
      end
    end

    def do_not_reverse_lookup=(do_not)
      case do_not
      when false, nil
        @do_not_reverse_lookup = false
      else
        @do_not_reverse_lookup = true
      end
    end
  end

  def connect_address
    local_address
  end

  def remote_address
    Addrinfo.new(getpeername)
  end
end

class IPSocket < BasicSocket
  __bind_method__ :addr, :IPSocket_addr
end

class TCPSocket < IPSocket
  __bind_method__ :initialize, :TCPSocket_initialize
end

class TCPServer < TCPSocket
  __bind_method__ :initialize, :TCPServer_initialize
  __bind_method__ :accept, :TCPServer_accept
  __bind_method__ :accept_nonblock, :TCPServer_accept_nonblock
  __bind_method__ :listen, :TCPServer_listen
  __bind_method__ :sysaccept, :TCPServer_sysaccept, 0
end

class UDPSocket < IPSocket
  __bind_method__ :initialize, :UDPSocket_initialize, 1
  __bind_method__ :bind, :UDPSocket_bind, 2
  __bind_method__ :connect, :UDPSocket_connect, 2
  __bind_method__ :recvfrom_nonblock, :UDPSocket_recvfrom_nonblock
end

class UNIXSocket < BasicSocket
  __bind_method__ :initialize, :UNIXSocket_initialize, 1
  __bind_method__ :addr, :UNIXSocket_addr, 0
  __bind_method__ :peeraddr, :UNIXSocket_peeraddr, 0
  __bind_method__ :recvfrom, :UNIXSocket_recvfrom

  def path
    addr[1]
  end

  class << self
    __bind_method__ :pair, :UNIXSocket_pair
    alias socketpair pair
  end
end

class UNIXServer < UNIXSocket
  __bind_method__ :initialize, :UNIXServer_initialize, 1
  __bind_method__ :accept, :UNIXServer_accept, 0
  __bind_method__ :accept_nonblock, :UNIXServer_accept_nonblock
  __bind_method__ :listen, :UNIXServer_listen, 1
  __bind_method__ :sysaccept, :UNIXServer_sysaccept, 0
end

require_relative './socket/constants'

class Socket < BasicSocket
  Constants.constants.each do |name|
    const_set(name, Constants.const_get(name))
  end

  SHORT_CONSTANTS = {
    DGRAM: SOCK_DGRAM,
    INET: AF_INET,
    INET6: AF_INET6,
    IP: IPPROTO_IP,
    IPV6: IPPROTO_IPV6,
    KEEPALIVE: SO_KEEPALIVE,
    LINGER: SO_LINGER,
    NODELAY: TCP_NODELAY,
    OOBINLINE: SO_OOBINLINE,
    REUSEADDR: SO_REUSEADDR,
    SOCKET: SOL_SOCKET,
    STREAM: SOCK_STREAM,
    TCP: IPPROTO_TCP,
    TTL: IP_TTL,
    TYPE: SO_TYPE,
    UDP: IPPROTO_UDP,
    UNIX: AF_UNIX,
    V6ONLY: IPV6_V6ONLY,
  }.freeze

  class Option
    def initialize(family, level, optname, data)
      @family = Socket.const_name_to_i(family)
      @level = Socket.const_name_to_i(level)
      @optname = Socket.const_name_to_i(optname)
      @data = data
    end

    attr_reader :family, :level, :optname, :data

    alias to_s data

    class << self
      def bool(family, level, optname, data)
        Option.new(family, level, optname, [data ? 1 : 0].pack('i'))
      end

      def int(family, level, optname, data)
        Option.new(family, level, optname, [data].pack('i'))
      end

      __bind_method__ :linger, :Socket_Option_s_linger
    end

    __bind_method__ :bool, :Socket_Option_bool
    __bind_method__ :int, :Socket_Option_int
    __bind_method__ :linger, :Socket_Option_linger
  end

  __bind_method__ :initialize, :Socket_initialize

  __bind_method__ :accept, :Socket_accept
  __bind_method__ :bind, :Socket_bind
  __bind_method__ :close, :Socket_close
  __bind_method__ :closed?, :Socket_is_closed
  __bind_method__ :connect, :Socket_connect
  __bind_method__ :listen, :Socket_listen

  class << self
    __bind_method__ :pair, :Socket_pair
    __bind_method__ :pack_sockaddr_in, :Socket_pack_sockaddr_in
    __bind_method__ :pack_sockaddr_un, :Socket_pack_sockaddr_un
    __bind_method__ :unpack_sockaddr_in, :Socket_unpack_sockaddr_in
    __bind_method__ :unpack_sockaddr_un, :Socket_unpack_sockaddr_un

    __bind_method__ :getaddrinfo, :Socket_s_getaddrinfo

    __bind_method__ :const_name_to_i, :Socket_const_name_to_i

    alias socketpair pair
    alias sockaddr_in pack_sockaddr_in
    alias sockaddr_un pack_sockaddr_un

    def tcp(host, port, local_host = nil, local_port = nil)
      block_given = block_given?
      Socket.new(:INET, :STREAM).tap do |socket|
        if local_host || local_port
          local_sockaddr = Socket.pack_sockaddr_in(
            local_port || host,
            local_host || port,
          )
          socket.bind(local_sockaddr)
        end
        sockaddr = Socket.pack_sockaddr_in(port, host)
        socket.connect(sockaddr)
        if block_given
          begin
            yield socket
          ensure
            socket.close
          end
        end
      end
    end

    def unix(path, &block)
      block_given = block_given?
      Socket.new(:UNIX, :STREAM).tap do |socket|
        sockaddr = Socket.pack_sockaddr_un(path)
        socket.connect(sockaddr)
        if block_given
          begin
            yield socket
          ensure
            socket.close
          end
        end
      end
    end

    def unix_server_socket(path, &block)
      block_given = block_given?
      Socket.new(:UNIX, :STREAM).tap do |socket|
        sockaddr = Socket.pack_sockaddr_un(path)
        socket.bind(sockaddr)
        if block_given
          begin
            yield socket
          ensure
            socket.close
          end
        end
      end
    end
  end
end

class Addrinfo
  attr_reader :afamily, :family, :pfamily, :protocol, :socktype, :canonname

  class << self
    __bind_method__ :getaddrinfo, :Addrinfo_getaddrinfo

    def ip(ip)
      Addrinfo.new(Socket.pack_sockaddr_in(0, ip), nil, nil, Socket::IPPROTO_IP)
    end

    def tcp(ip, port)
      Addrinfo.new(Socket.pack_sockaddr_in(port, ip), nil, Socket::SOCK_STREAM, Socket::IPPROTO_TCP)
    end

    def udp(ip, port)
      Addrinfo.new(Socket.pack_sockaddr_in(port, ip), nil, Socket::SOCK_DGRAM, Socket::IPPROTO_UDP)
    end

    def unix(path)
      Addrinfo.new([:UNIX, path])
    end
  end

  __bind_method__ :initialize, :Addrinfo_initialize
  __bind_method__ :to_sockaddr, :Addrinfo_to_sockaddr

  alias to_s to_sockaddr

  def bind
    socket = Socket.new(afamily, socktype, protocol)
    socket.bind(to_sockaddr)
    socket
  end

  def connect
    socket = Socket.new(afamily, socktype, protocol)
    socket.connect(to_sockaddr)
    if block_given?
      begin
        yield socket
      ensure
        socket.close
      end
    else
      socket
    end
  end

  def ip_address
    unless @ip_address
      raise SocketError, 'need IPv4 or IPv6 address'
    end
    @ip_address
  end

  def ip_port
    unless @ip_port
      raise SocketError, 'need IPv4 or IPv6 address'
    end
    @ip_port
  end

  def ip_unpack
    [ip_address, ip_port]
  end

  def ipv4?
    afamily == Socket::AF_INET
  end

  def ipv6?
    afamily == Socket::AF_INET6
  end

  def ip?
    ipv4? || ipv6?
  end

  def unix?
    afamily == Socket::AF_UNIX
  end

  def unix_path
    raise SocketError, 'need AF_UNIX address' unless unix?
    @unix_path
  end
end
