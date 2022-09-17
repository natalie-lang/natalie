require 'natalie/inline'
require 'socket.cpp'

class SocketError < StandardError
end

class BasicSocket < IO
  __bind_method__ :getsockopt, :BasicSocket_getsockopt
  __bind_method__ :setsockopt, :BasicSocket_setsockopt

  attr_reader :local_address

  attr_writer :do_not_reverse_lookup

  def do_not_reverse_lookup
    if @do_not_reverse_lookup.nil?
      self.class.do_not_reverse_lookup
    else
      @do_not_reverse_lookup
    end
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
end

class IPSocket < BasicSocket
  __bind_method__ :addr, :IPSocket_addr
end

class TCPSocket < IPSocket

end

class TCPServer < TCPSocket
  __bind_method__ :initialize, :TCPServer_initialize
end

class Socket < BasicSocket
  AF_APPLETALK = __constant__('AF_APPLETALK', 'unsigned short')
  AF_AX25 = __constant__('AF_AX25', 'unsigned short')
  AF_INET = __constant__('AF_INET', 'unsigned short')
  AF_INET6 = __constant__('AF_INET6', 'unsigned short')
  AF_IPX = __constant__('AF_IPX', 'unsigned short')
  AF_ISDN = __constant__('AF_ISDN', 'unsigned short')
  AF_LOCAL = __constant__('AF_LOCAL', 'unsigned short')
  AF_MAX = __constant__('AF_MAX', 'unsigned short')
  AF_PACKET = __constant__('AF_PACKET', 'unsigned short')
  AF_ROUTE = __constant__('AF_ROUTE', 'unsigned short')
  AF_SNA = __constant__('AF_SNA', 'unsigned short')
  AF_UNIX = __constant__('AF_UNIX', 'unsigned short')
  AF_UNSPEC = __constant__('AF_UNSPEC', 'unsigned short')
  AI_ADDRCONFIG = __constant__('AI_ADDRCONFIG', 'unsigned short')
  AI_ALL = __constant__('AI_ALL', 'unsigned short')
  AI_CANONNAME = __constant__('AI_CANONNAME', 'unsigned short')
  AI_NUMERICHOST = __constant__('AI_NUMERICHOST', 'unsigned short')
  AI_NUMERICSERV = __constant__('AI_NUMERICSERV', 'unsigned short')
  AI_PASSIVE = __constant__('AI_PASSIVE', 'unsigned short')
  AI_V4MAPPED = __constant__('AI_V4MAPPED', 'unsigned short')
  EAI_ADDRFAMILY = __constant__('EAI_ADDRFAMILY', 'unsigned short')
  EAI_AGAIN = __constant__('EAI_AGAIN', 'unsigned short')
  EAI_BADFLAGS = __constant__('EAI_BADFLAGS', 'unsigned short')
  EAI_FAIL = __constant__('EAI_FAIL', 'unsigned short')
  EAI_FAMILY = __constant__('EAI_FAMILY', 'unsigned short')
  EAI_MEMORY = __constant__('EAI_MEMORY', 'unsigned short')
  EAI_NODATA = __constant__('EAI_NODATA', 'unsigned short')
  EAI_NONAME = __constant__('EAI_NONAME', 'unsigned short')
  EAI_OVERFLOW = __constant__('EAI_OVERFLOW', 'unsigned short')
  EAI_SERVICE = __constant__('EAI_SERVICE', 'unsigned short')
  EAI_SOCKTYPE = __constant__('EAI_SOCKTYPE', 'unsigned short')
  EAI_SYSTEM = __constant__('EAI_SYSTEM', 'unsigned short')
  INADDR_ALLHOSTS_GROUP = __constant__('INADDR_ALLHOSTS_GROUP', 'unsigned short')
  INADDR_ANY = __constant__('INADDR_ANY', 'unsigned short')
  INADDR_BROADCAST = __constant__('INADDR_BROADCAST', 'unsigned short')
  INADDR_LOOPBACK = __constant__('INADDR_LOOPBACK', 'unsigned short')
  INADDR_MAX_LOCAL_GROUP = __constant__('INADDR_MAX_LOCAL_GROUP', 'unsigned short')
  INADDR_NONE = __constant__('INADDR_NONE', 'unsigned short')
  INADDR_UNSPEC_GROUP = __constant__('INADDR_UNSPEC_GROUP', 'unsigned short')
  INET6_ADDRSTRLEN = __constant__('INET6_ADDRSTRLEN', 'unsigned short')
  INET_ADDRSTRLEN = __constant__('INET_ADDRSTRLEN', 'unsigned short')
  IPPORT_RESERVED = __constant__('IPPORT_RESERVED', 'unsigned short')
  IPPORT_USERRESERVED = __constant__('IPPORT_USERRESERVED', 'unsigned short')
  IPPROTO_AH = __constant__('IPPROTO_AH', 'unsigned short')
  IPPROTO_DSTOPTS = __constant__('IPPROTO_DSTOPTS', 'unsigned short')
  IPPROTO_EGP = __constant__('IPPROTO_EGP', 'unsigned short')
  IPPROTO_ESP = __constant__('IPPROTO_ESP', 'unsigned short')
  IPPROTO_FRAGMENT = __constant__('IPPROTO_FRAGMENT', 'unsigned short')
  IPPROTO_HOPOPTS = __constant__('IPPROTO_HOPOPTS', 'unsigned short')
  IPPROTO_ICMP = __constant__('IPPROTO_ICMP', 'unsigned short')
  IPPROTO_ICMPV6 = __constant__('IPPROTO_ICMPV6', 'unsigned short')
  IPPROTO_IDP = __constant__('IPPROTO_IDP', 'unsigned short')
  IPPROTO_IGMP = __constant__('IPPROTO_IGMP', 'unsigned short')
  IPPROTO_IP = __constant__('IPPROTO_IP', 'unsigned short')
  IPPROTO_IPV6 = __constant__('IPPROTO_IPV6', 'unsigned short')
  IPPROTO_NONE = __constant__('IPPROTO_NONE', 'unsigned short')
  IPPROTO_PUP = __constant__('IPPROTO_PUP', 'unsigned short')
  IPPROTO_RAW = __constant__('IPPROTO_RAW', 'unsigned short')
  IPPROTO_ROUTING = __constant__('IPPROTO_ROUTING', 'unsigned short')
  IPPROTO_TCP = __constant__('IPPROTO_TCP', 'unsigned short')
  IPPROTO_TP = __constant__('IPPROTO_TP', 'unsigned short')
  IPPROTO_UDP = __constant__('IPPROTO_UDP', 'unsigned short')
  IPV6_CHECKSUM = __constant__('IPV6_CHECKSUM', 'unsigned short')
  IPV6_DONTFRAG = __constant__('IPV6_DONTFRAG', 'unsigned short')
  IPV6_DSTOPTS = __constant__('IPV6_DSTOPTS', 'unsigned short')
  IPV6_HOPLIMIT = __constant__('IPV6_HOPLIMIT', 'unsigned short')
  IPV6_HOPOPTS = __constant__('IPV6_HOPOPTS', 'unsigned short')
  IPV6_JOIN_GROUP = __constant__('IPV6_JOIN_GROUP', 'unsigned short')
  IPV6_LEAVE_GROUP = __constant__('IPV6_LEAVE_GROUP', 'unsigned short')
  IPV6_MULTICAST_HOPS = __constant__('IPV6_MULTICAST_HOPS', 'unsigned short')
  IPV6_MULTICAST_IF = __constant__('IPV6_MULTICAST_IF', 'unsigned short')
  IPV6_MULTICAST_LOOP = __constant__('IPV6_MULTICAST_LOOP', 'unsigned short')
  IPV6_NEXTHOP = __constant__('IPV6_NEXTHOP', 'unsigned short')
  IPV6_PATHMTU = __constant__('IPV6_PATHMTU', 'unsigned short')
  IPV6_PKTINFO = __constant__('IPV6_PKTINFO', 'unsigned short')
  IPV6_RECVDSTOPTS = __constant__('IPV6_RECVDSTOPTS', 'unsigned short')
  IPV6_RECVHOPLIMIT = __constant__('IPV6_RECVHOPLIMIT', 'unsigned short')
  IPV6_RECVHOPOPTS = __constant__('IPV6_RECVHOPOPTS', 'unsigned short')
  IPV6_RECVPATHMTU = __constant__('IPV6_RECVPATHMTU', 'unsigned short')
  IPV6_RECVPKTINFO = __constant__('IPV6_RECVPKTINFO', 'unsigned short')
  IPV6_RECVRTHDR = __constant__('IPV6_RECVRTHDR', 'unsigned short')
  IPV6_RECVTCLASS = __constant__('IPV6_RECVTCLASS', 'unsigned short')
  IPV6_RTHDR = __constant__('IPV6_RTHDR', 'unsigned short')
  IPV6_RTHDRDSTOPTS = __constant__('IPV6_RTHDRDSTOPTS', 'unsigned short')
  IPV6_RTHDR_TYPE_0 = __constant__('IPV6_RTHDR_TYPE_0', 'unsigned short')
  IPV6_TCLASS = __constant__('IPV6_TCLASS', 'unsigned short')
  IPV6_UNICAST_HOPS = __constant__('IPV6_UNICAST_HOPS', 'unsigned short')
  IPV6_V6ONLY = __constant__('IPV6_V6ONLY', 'unsigned short')
  IP_ADD_MEMBERSHIP = __constant__('IP_ADD_MEMBERSHIP', 'unsigned short')
  IP_ADD_SOURCE_MEMBERSHIP = __constant__('IP_ADD_SOURCE_MEMBERSHIP', 'unsigned short')
  IP_BLOCK_SOURCE = __constant__('IP_BLOCK_SOURCE', 'unsigned short')
  IP_DEFAULT_MULTICAST_LOOP = __constant__('IP_DEFAULT_MULTICAST_LOOP', 'unsigned short')
  IP_DEFAULT_MULTICAST_TTL = __constant__('IP_DEFAULT_MULTICAST_TTL', 'unsigned short')
  IP_DROP_MEMBERSHIP = __constant__('IP_DROP_MEMBERSHIP', 'unsigned short')
  IP_DROP_SOURCE_MEMBERSHIP = __constant__('IP_DROP_SOURCE_MEMBERSHIP', 'unsigned short')
  IP_FREEBIND = __constant__('IP_FREEBIND', 'unsigned short')
  IP_HDRINCL = __constant__('IP_HDRINCL', 'unsigned short')
  IP_IPSEC_POLICY = __constant__('IP_IPSEC_POLICY', 'unsigned short')
  IP_MAX_MEMBERSHIPS = __constant__('IP_MAX_MEMBERSHIPS', 'unsigned short')
  IP_MINTTL = __constant__('IP_MINTTL', 'unsigned short')
  IP_MSFILTER = __constant__('IP_MSFILTER', 'unsigned short')
  IP_MTU = __constant__('IP_MTU', 'unsigned short')
  IP_MTU_DISCOVER = __constant__('IP_MTU_DISCOVER', 'unsigned short')
  IP_MULTICAST_IF = __constant__('IP_MULTICAST_IF', 'unsigned short')
  IP_MULTICAST_LOOP = __constant__('IP_MULTICAST_LOOP', 'unsigned short')
  IP_MULTICAST_TTL = __constant__('IP_MULTICAST_TTL', 'unsigned short')
  IP_OPTIONS = __constant__('IP_OPTIONS', 'unsigned short')
  IP_PASSSEC = __constant__('IP_PASSSEC', 'unsigned short')
  IP_PKTINFO = __constant__('IP_PKTINFO', 'unsigned short')
  IP_PKTOPTIONS = __constant__('IP_PKTOPTIONS', 'unsigned short')
  IP_PMTUDISC_DO = __constant__('IP_PMTUDISC_DO', 'unsigned short')
  IP_PMTUDISC_DONT = __constant__('IP_PMTUDISC_DONT', 'unsigned short')
  IP_PMTUDISC_WANT = __constant__('IP_PMTUDISC_WANT', 'unsigned short')
  IP_RECVERR = __constant__('IP_RECVERR', 'unsigned short')
  IP_RECVOPTS = __constant__('IP_RECVOPTS', 'unsigned short')
  IP_RECVRETOPTS = __constant__('IP_RECVRETOPTS', 'unsigned short')
  IP_RECVTOS = __constant__('IP_RECVTOS', 'unsigned short')
  IP_RECVTTL = __constant__('IP_RECVTTL', 'unsigned short')
  IP_RETOPTS = __constant__('IP_RETOPTS', 'unsigned short')
  IP_ROUTER_ALERT = __constant__('IP_ROUTER_ALERT', 'unsigned short')
  IP_TOS = __constant__('IP_TOS', 'unsigned short')
  IP_TRANSPARENT = __constant__('IP_TRANSPARENT', 'unsigned short')
  IP_TTL = __constant__('IP_TTL', 'unsigned short')
  IP_UNBLOCK_SOURCE = __constant__('IP_UNBLOCK_SOURCE', 'unsigned short')
  IP_XFRM_POLICY = __constant__('IP_XFRM_POLICY', 'unsigned short')
  LOCK_EX = __constant__('LOCK_EX', 'unsigned short')
  LOCK_NB = __constant__('LOCK_NB', 'unsigned short')
  LOCK_SH = __constant__('LOCK_SH', 'unsigned short')
  LOCK_UN = __constant__('LOCK_UN', 'unsigned short')
  MCAST_BLOCK_SOURCE = __constant__('MCAST_BLOCK_SOURCE', 'unsigned short')
  MCAST_EXCLUDE = __constant__('MCAST_EXCLUDE', 'unsigned short')
  MCAST_INCLUDE = __constant__('MCAST_INCLUDE', 'unsigned short')
  MCAST_JOIN_GROUP = __constant__('MCAST_JOIN_GROUP', 'unsigned short')
  MCAST_JOIN_SOURCE_GROUP = __constant__('MCAST_JOIN_SOURCE_GROUP', 'unsigned short')
  MCAST_LEAVE_GROUP = __constant__('MCAST_LEAVE_GROUP', 'unsigned short')
  MCAST_LEAVE_SOURCE_GROUP = __constant__('MCAST_LEAVE_SOURCE_GROUP', 'unsigned short')
  MCAST_MSFILTER = __constant__('MCAST_MSFILTER', 'unsigned short')
  MCAST_UNBLOCK_SOURCE = __constant__('MCAST_UNBLOCK_SOURCE', 'unsigned short')
  MSG_CONFIRM = __constant__('MSG_CONFIRM', 'unsigned short')
  MSG_CTRUNC = __constant__('MSG_CTRUNC', 'unsigned short')
  MSG_DONTROUTE = __constant__('MSG_DONTROUTE', 'unsigned short')
  MSG_DONTWAIT = __constant__('MSG_DONTWAIT', 'unsigned short')
  MSG_EOR = __constant__('MSG_EOR', 'unsigned short')
  MSG_ERRQUEUE = __constant__('MSG_ERRQUEUE', 'unsigned short')
  MSG_FASTOPEN = __constant__('MSG_FASTOPEN', 'unsigned short')
  MSG_FIN = __constant__('MSG_FIN', 'unsigned short')
  MSG_MORE = __constant__('MSG_MORE', 'unsigned short')
  MSG_NOSIGNAL = __constant__('MSG_NOSIGNAL', 'unsigned short')
  MSG_OOB = __constant__('MSG_OOB', 'unsigned short')
  MSG_PEEK = __constant__('MSG_PEEK', 'unsigned short')
  MSG_PROXY = __constant__('MSG_PROXY', 'unsigned short')
  MSG_RST = __constant__('MSG_RST', 'unsigned short')
  MSG_SYN = __constant__('MSG_SYN', 'unsigned short')
  MSG_TRUNC = __constant__('MSG_TRUNC', 'unsigned short')
  MSG_WAITALL = __constant__('MSG_WAITALL', 'unsigned short')
  NI_DGRAM = __constant__('NI_DGRAM', 'unsigned short')
  NI_MAXHOST = __constant__('NI_MAXHOST', 'unsigned short')
  NI_MAXSERV = __constant__('NI_MAXSERV', 'unsigned short')
  NI_NAMEREQD = __constant__('NI_NAMEREQD', 'unsigned short')
  NI_NOFQDN = __constant__('NI_NOFQDN', 'unsigned short')
  NI_NUMERICHOST = __constant__('NI_NUMERICHOST', 'unsigned short')
  NI_NUMERICSERV = __constant__('NI_NUMERICSERV', 'unsigned short')
  PF_APPLETALK = __constant__('PF_APPLETALK', 'unsigned short')
  PF_AX25 = __constant__('PF_AX25', 'unsigned short')
  PF_INET = __constant__('PF_INET', 'unsigned short')
  PF_INET6 = __constant__('PF_INET6', 'unsigned short')
  PF_IPX = __constant__('PF_IPX', 'unsigned short')
  PF_ISDN = __constant__('PF_ISDN', 'unsigned short')
  PF_KEY = __constant__('PF_KEY', 'unsigned short')
  PF_LOCAL = __constant__('PF_LOCAL', 'unsigned short')
  PF_MAX = __constant__('PF_MAX', 'unsigned short')
  PF_PACKET = __constant__('PF_PACKET', 'unsigned short')
  PF_ROUTE = __constant__('PF_ROUTE', 'unsigned short')
  PF_SNA = __constant__('PF_SNA', 'unsigned short')
  PF_UNIX = __constant__('PF_UNIX', 'unsigned short')
  PF_UNSPEC = __constant__('PF_UNSPEC', 'unsigned short')
  SCM_CREDENTIALS = __constant__('SCM_CREDENTIALS', 'unsigned short')
  SCM_RIGHTS = __constant__('SCM_RIGHTS', 'unsigned short')
  SCM_TIMESTAMP = __constant__('SCM_TIMESTAMP', 'unsigned short')
  SCM_TIMESTAMPING = __constant__('SCM_TIMESTAMPING', 'unsigned short')
  SCM_TIMESTAMPNS = __constant__('SCM_TIMESTAMPNS', 'unsigned short')
  SCM_WIFI_STATUS = __constant__('SCM_WIFI_STATUS', 'unsigned short')
  SEEK_CUR = __constant__('SEEK_CUR', 'unsigned short')
  SEEK_DATA = __constant__('SEEK_DATA', 'unsigned short')
  SEEK_END = __constant__('SEEK_END', 'unsigned short')
  SEEK_HOLE = __constant__('SEEK_HOLE', 'unsigned short')
  SEEK_SET = __constant__('SEEK_SET', 'unsigned short')
  SHUT_RD = __constant__('SHUT_RD', 'unsigned short')
  SHUT_RDWR = __constant__('SHUT_RDWR', 'unsigned short')
  SHUT_WR = __constant__('SHUT_WR', 'unsigned short')
  SOCK_DGRAM = __constant__('SOCK_DGRAM', 'unsigned short')
  SOCK_PACKET = __constant__('SOCK_PACKET', 'unsigned short')
  SOCK_RAW = __constant__('SOCK_RAW', 'unsigned short')
  SOCK_RDM = __constant__('SOCK_RDM', 'unsigned short')
  SOCK_SEQPACKET = __constant__('SOCK_SEQPACKET', 'unsigned short')
  SOCK_STREAM = __constant__('SOCK_STREAM', 'unsigned short')
  SOL_IP = __constant__('SOL_IP', 'unsigned short')
  SOL_SOCKET = __constant__('SOL_SOCKET', 'unsigned short')
  SOMAXCONN = __constant__('SOMAXCONN', 'unsigned short')
  SO_ACCEPTCONN = __constant__('SO_ACCEPTCONN', 'unsigned short')
  SO_ATTACH_FILTER = __constant__('SO_ATTACH_FILTER', 'unsigned short')
  SO_BINDTODEVICE = __constant__('SO_BINDTODEVICE', 'unsigned short')
  SO_BPF_EXTENSIONS = __constant__('SO_BPF_EXTENSIONS', 'unsigned short')
  SO_BROADCAST = __constant__('SO_BROADCAST', 'unsigned short')
  SO_BUSY_POLL = __constant__('SO_BUSY_POLL', 'unsigned short')
  SO_DEBUG = __constant__('SO_DEBUG', 'unsigned short')
  SO_DETACH_FILTER = __constant__('SO_DETACH_FILTER', 'unsigned short')
  SO_DOMAIN = __constant__('SO_DOMAIN', 'unsigned short')
  SO_DONTROUTE = __constant__('SO_DONTROUTE', 'unsigned short')
  SO_ERROR = __constant__('SO_ERROR', 'unsigned short')
  SO_GET_FILTER = __constant__('SO_GET_FILTER', 'unsigned short')
  SO_KEEPALIVE = __constant__('SO_KEEPALIVE', 'unsigned short')
  SO_LINGER = __constant__('SO_LINGER', 'unsigned short')
  SO_LOCK_FILTER = __constant__('SO_LOCK_FILTER', 'unsigned short')
  SO_MARK = __constant__('SO_MARK', 'unsigned short')
  SO_MAX_PACING_RATE = __constant__('SO_MAX_PACING_RATE', 'unsigned short')
  SO_NOFCS = __constant__('SO_NOFCS', 'unsigned short')
  SO_NO_CHECK = __constant__('SO_NO_CHECK', 'unsigned short')
  SO_OOBINLINE = __constant__('SO_OOBINLINE', 'unsigned short')
  SO_PASSCRED = __constant__('SO_PASSCRED', 'unsigned short')
  SO_PASSSEC = __constant__('SO_PASSSEC', 'unsigned short')
  SO_PEEK_OFF = __constant__('SO_PEEK_OFF', 'unsigned short')
  SO_PEERCRED = __constant__('SO_PEERCRED', 'unsigned short')
  SO_PEERNAME = __constant__('SO_PEERNAME', 'unsigned short')
  SO_PEERSEC = __constant__('SO_PEERSEC', 'unsigned short')
  SO_PRIORITY = __constant__('SO_PRIORITY', 'unsigned short')
  SO_PROTOCOL = __constant__('SO_PROTOCOL', 'unsigned short')
  SO_RCVBUF = __constant__('SO_RCVBUF', 'unsigned short')
  SO_RCVBUFFORCE = __constant__('SO_RCVBUFFORCE', 'unsigned short')
  SO_RCVLOWAT = __constant__('SO_RCVLOWAT', 'unsigned short')
  SO_RCVTIMEO = __constant__('SO_RCVTIMEO', 'unsigned short')
  SO_REUSEADDR = __constant__('SO_REUSEADDR', 'unsigned short')
  SO_REUSEPORT = __constant__('SO_REUSEPORT', 'unsigned short')
  SO_RXQ_OVFL = __constant__('SO_RXQ_OVFL', 'unsigned short')
  SO_SECURITY_AUTHENTICATION = __constant__('SO_SECURITY_AUTHENTICATION', 'unsigned short')
  SO_SECURITY_ENCRYPTION_NETWORK = __constant__('SO_SECURITY_ENCRYPTION_NETWORK', 'unsigned short')
  SO_SECURITY_ENCRYPTION_TRANSPORT = __constant__('SO_SECURITY_ENCRYPTION_TRANSPORT', 'unsigned short')
  SO_SELECT_ERR_QUEUE = __constant__('SO_SELECT_ERR_QUEUE', 'unsigned short')
  SO_SNDBUF = __constant__('SO_SNDBUF', 'unsigned short')
  SO_SNDBUFFORCE = __constant__('SO_SNDBUFFORCE', 'unsigned short')
  SO_SNDLOWAT = __constant__('SO_SNDLOWAT', 'unsigned short')
  SO_SNDTIMEO = __constant__('SO_SNDTIMEO', 'unsigned short')
  SO_TIMESTAMP = __constant__('SO_TIMESTAMP', 'unsigned short')
  SO_TIMESTAMPING = __constant__('SO_TIMESTAMPING', 'unsigned short')
  SO_TIMESTAMPNS = __constant__('SO_TIMESTAMPNS', 'unsigned short')
  SO_TYPE = __constant__('SO_TYPE', 'unsigned short')
  SO_WIFI_STATUS = __constant__('SO_WIFI_STATUS', 'unsigned short')

  SHORT_CONSTANTS = {
    DGRAM: SOCK_DGRAM,
    INET: AF_INET,
    INET6: AF_INET6,
    IP: IPPROTO_IP,
    KEEPALIVE: SO_KEEPALIVE,
    LINGER: SO_LINGER,
    OOBINLINE: SO_OOBINLINE,
    REUSEADDR: SO_REUSEADDR,
    SOCKET: SOL_SOCKET,
    STREAM: SOCK_STREAM,
    TTL: IP_TTL,
    TYPE: SO_TYPE,
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

  __bind_method__ :bind, :Socket_bind
  __bind_method__ :close, :Socket_close
  __bind_method__ :closed?, :Socket_is_closed
  __bind_method__ :connect, :Socket_connect
  __bind_method__ :listen, :Socket_listen

  class << self
    __bind_method__ :pack_sockaddr_in, :Socket_pack_sockaddr_in
    __bind_method__ :pack_sockaddr_un, :Socket_pack_sockaddr_un
    __bind_method__ :unpack_sockaddr_in, :Socket_unpack_sockaddr_in
    __bind_method__ :unpack_sockaddr_un, :Socket_unpack_sockaddr_un

    __bind_method__ :getaddrinfo, :Socket_s_getaddrinfo

    __bind_method__ :const_name_to_i, :Socket_const_name_to_i

    alias sockaddr_in pack_sockaddr_in
    alias sockaddr_un pack_sockaddr_un

    def tcp(host, port, local_host = nil, local_port = nil)
      block_given = block_given?
      Socket.new(:INET, :STREAM).tap do |socket|
        sockaddr = Socket.pack_sockaddr_in(
          local_port || port,
          local_host || host,
        )
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
  end
end

class Addrinfo
  attr_reader :afamily, :family, :pfamily, :protocol, :socktype, :unix_path

  class << self
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
      Addrinfo.new(Socket.pack_sockaddr_un(path))
    end
  end

  __bind_method__ :initialize, :Addrinfo_initialize
  __bind_method__ :to_sockaddr, :Addrinfo_to_sockaddr

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
end
