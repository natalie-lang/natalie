# frozen_string_literal: true
#
# Resolv is a thread-aware DNS resolver library written in Ruby.  Resolv can
# handle multiple DNS requests concurrently without blocking the entire Ruby
# interpreter.
#
# See also resolv-replace.rb to replace the libc resolver with Resolv.
#
# Resolv can look up various DNS resources using the DNS module directly.
#
# Examples:
#
#   p Resolv.getaddress "www.ruby-lang.org"
#   p Resolv.getname "210.251.121.214"
#
#   Resolv::DNS.open do |dns|
#     ress = dns.getresources "www.ruby-lang.org", Resolv::DNS::Resource::IN::A
#     p ress.map(&:address)
#     ress = dns.getresources "ruby-lang.org", Resolv::DNS::Resource::IN::MX
#     p ress.map { |r| [r.exchange.to_s, r.preference] }
#   end
#
#
# == Bugs
#
# * NIS is not supported.
# * /etc/nsswitch.conf is not supported.

class Resolv

  VERSION = "0.3.0"

  ##
  # A Resolv::DNS IPv4 address.

  class IPv4

    ##
    # Regular expression IPv4 addresses must match.

    Regex256 = /0
               |1(?:[0-9][0-9]?)?
               |2(?:[0-4][0-9]?|5[0-5]?|[6-9])?
               |[3-9][0-9]?/x
    Regex = /\A(#{Regex256})\.(#{Regex256})\.(#{Regex256})\.(#{Regex256})\z/

    def self.create(arg)
      case arg
      when IPv4
        return arg
      when Regex
        if (0..255) === (a = $1.to_i) &&
           (0..255) === (b = $2.to_i) &&
           (0..255) === (c = $3.to_i) &&
           (0..255) === (d = $4.to_i)
          return self.new([a, b, c, d].pack("CCCC"))
        else
          raise ArgumentError.new("IPv4 address with invalid value: " + arg)
        end
      else
        raise ArgumentError.new("cannot interpret as IPv4 address: #{arg.inspect}")
      end
    end

    def initialize(address) # :nodoc:
      unless address.kind_of?(String)
        raise ArgumentError, 'IPv4 address must be a string'
      end
      unless address.length == 4
        raise ArgumentError, "IPv4 address expects 4 bytes but #{address.length} bytes"
      end
      @address = address
    end

    ##
    # A String representation of this IPv4 address.

    ##
    # The raw IPv4 address as a String.

    attr_reader :address

    def to_s # :nodoc:
      return sprintf("%d.%d.%d.%d", *@address.unpack("CCCC"))
    end

    def inspect # :nodoc:
      return "#<#{self.class} #{self}>"
    end

    ##
    # Turns this IPv4 address into a Resolv::DNS::Name.

    def to_name
      return DNS::Name.create(
        '%d.%d.%d.%d.in-addr.arpa.' % @address.unpack('CCCC').reverse)
    end

    def ==(other) # :nodoc:
      return @address == other.address
    end

    def eql?(other) # :nodoc:
      return self == other
    end

    def hash # :nodoc:
      return @address.hash
    end
  end

  ##
  # A Resolv::DNS IPv6 address.

  class IPv6

    ##
    # IPv6 address format a:b:c:d:e:f:g:h
    Regex_8Hex = /\A
      (?:[0-9A-Fa-f]{1,4}:){7}
         [0-9A-Fa-f]{1,4}
      \z/x

    ##
    # Compressed IPv6 address format a::b

    Regex_CompressedHex = /\A
      ((?:[0-9A-Fa-f]{1,4}(?::[0-9A-Fa-f]{1,4})*)?) ::
      ((?:[0-9A-Fa-f]{1,4}(?::[0-9A-Fa-f]{1,4})*)?)
      \z/x

    ##
    # IPv4 mapped IPv6 address format a:b:c:d:e:f:w.x.y.z

    Regex_6Hex4Dec = /\A
      ((?:[0-9A-Fa-f]{1,4}:){6,6})
      (\d+)\.(\d+)\.(\d+)\.(\d+)
      \z/x

    ##
    # Compressed IPv4 mapped IPv6 address format a::b:w.x.y.z

    Regex_CompressedHex4Dec = /\A
      ((?:[0-9A-Fa-f]{1,4}(?::[0-9A-Fa-f]{1,4})*)?) ::
      ((?:[0-9A-Fa-f]{1,4}:)*)
      (\d+)\.(\d+)\.(\d+)\.(\d+)
      \z/x

    ##
    # IPv6 link local address format fe80:b:c:d:e:f:g:h%em1
    Regex_8HexLinkLocal = /\A
      [Ff][Ee]80
      (?::[0-9A-Fa-f]{1,4}){7}
      %[-0-9A-Za-z._~]+
      \z/x

    ##
    # Compressed IPv6 link local address format fe80::b%em1

    Regex_CompressedHexLinkLocal = /\A
      [Ff][Ee]80:
      (?:
        ((?:[0-9A-Fa-f]{1,4}(?::[0-9A-Fa-f]{1,4})*)?) ::
        ((?:[0-9A-Fa-f]{1,4}(?::[0-9A-Fa-f]{1,4})*)?)
        |
        :((?:[0-9A-Fa-f]{1,4}(?::[0-9A-Fa-f]{1,4})*)?)
      )?
      :[0-9A-Fa-f]{1,4}%[-0-9A-Za-z._~]+
      \z/x

    ##
    # A composite IPv6 address Regexp.

    Regex = /
      (?:#{Regex_8Hex}) |
      (?:#{Regex_CompressedHex}) |
      (?:#{Regex_6Hex4Dec}) |
      (?:#{Regex_CompressedHex4Dec}) |
      (?:#{Regex_8HexLinkLocal}) |
      (?:#{Regex_CompressedHexLinkLocal})
      /x

    ##
    # Creates a new IPv6 address from +arg+ which may be:
    #
    # IPv6:: returns +arg+.
    # String:: +arg+ must match one of the IPv6::Regex* constants

    def self.create(arg)
      case arg
      when IPv6
        return arg
      when String
        address = ''.b
        if Regex_8Hex =~ arg
          arg.scan(/[0-9A-Fa-f]+/) {|hex| address << [hex.hex].pack('n')}
        elsif Regex_CompressedHex =~ arg
          prefix = $1
          suffix = $2
          a1 = ''.b
          a2 = ''.b
          prefix.scan(/[0-9A-Fa-f]+/) {|hex| a1 << [hex.hex].pack('n')}
          suffix.scan(/[0-9A-Fa-f]+/) {|hex| a2 << [hex.hex].pack('n')}
          omitlen = 16 - a1.length - a2.length
          address << a1 << "\0" * omitlen << a2
        elsif Regex_6Hex4Dec =~ arg
          prefix, a, b, c, d = $1, $2.to_i, $3.to_i, $4.to_i, $5.to_i
          if (0..255) === a && (0..255) === b && (0..255) === c && (0..255) === d
            prefix.scan(/[0-9A-Fa-f]+/) {|hex| address << [hex.hex].pack('n')}
            address << [a, b, c, d].pack('CCCC')
          else
            raise ArgumentError.new("not numeric IPv6 address: " + arg)
          end
        elsif Regex_CompressedHex4Dec =~ arg
          prefix, suffix, a, b, c, d = $1, $2, $3.to_i, $4.to_i, $5.to_i, $6.to_i
          if (0..255) === a && (0..255) === b && (0..255) === c && (0..255) === d
            a1 = ''.b
            a2 = ''.b
            prefix.scan(/[0-9A-Fa-f]+/) {|hex| a1 << [hex.hex].pack('n')}
            suffix.scan(/[0-9A-Fa-f]+/) {|hex| a2 << [hex.hex].pack('n')}
            omitlen = 12 - a1.length - a2.length
            address << a1 << "\0" * omitlen << a2 << [a, b, c, d].pack('CCCC')
          else
            raise ArgumentError.new("not numeric IPv6 address: " + arg)
          end
        else
          raise ArgumentError.new("not numeric IPv6 address: " + arg)
        end
        return IPv6.new(address)
      else
        raise ArgumentError.new("cannot interpret as IPv6 address: #{arg.inspect}")
      end
    end

    def initialize(address) # :nodoc:
      unless address.kind_of?(String) && address.length == 16
        raise ArgumentError.new('IPv6 address must be 16 bytes')
      end
      @address = address
    end

    ##
    # The raw IPv6 address as a String.

    attr_reader :address

    def to_s # :nodoc:
      sprintf("%x:%x:%x:%x:%x:%x:%x:%x", *@address.unpack("nnnnnnnn")).sub(/(^|:)0(:0)+(:|$)/, '::')
    end

    def inspect # :nodoc:
      return "#<#{self.class} #{self}>"
    end

    ##
    # Turns this IPv6 address into a Resolv::DNS::Name.
    #--
    # ip6.arpa should be searched too. [RFC3152]

    def to_name
      return DNS::Name.new(
        @address.unpack("H32")[0].split(//).reverse + ['ip6', 'arpa'])
    end

    def ==(other) # :nodoc:
      return @address == other.address
    end

    def eql?(other) # :nodoc:
      return self == other
    end

    def hash # :nodoc:
      return @address.hash
    end
  end
end
