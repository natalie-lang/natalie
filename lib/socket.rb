require 'natalie/inline'
require 'socket.cpp'

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
    __bind_method__ :pack_sockaddr_in, :Socket_pack_sockaddr_in
    __bind_method__ :pack_sockaddr_un, :Socket_pack_sockaddr_un
    __bind_method__ :unpack_sockaddr_in, :Socket_unpack_sockaddr_in
    __bind_method__ :unpack_sockaddr_un, :Socket_unpack_sockaddr_un

    alias sockaddr_in pack_sockaddr_in
    alias sockaddr_un pack_sockaddr_un
  end
end

class Addrinfo
  attr_reader :sockaddr, :family, :protocol

  class << self
    def tcp(ip, port)
      Addrinfo.new(Socket.pack_sockaddr_in(port, ip), nil, nil, Socket::IPPROTO_TCP)
    end

    def udp(_ip, _port)
      Addrinfo.new # TODO
    end

    def unix(path)
      Addrinfo.new(Socket.pack_sockaddr_un(path))
    end
  end

  __bind_method__ :initialize, :Addrinfo_initialize
  __bind_method__ :ip_address, :Addrinfo_ip_address
  __bind_method__ :pfamily, :Addrinfo_pfamily
  __bind_method__ :socktype, :Addrinfo_socktype

  def afamily
    # TODO
  end

  def unix_path
    # TODO
  end

  def ip_port
    # TODO
  end
end
