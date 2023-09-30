require 'natalie/inline'
require 'openssl.cpp'

__ld_flags__ '-lcrypto'

module OpenSSL
  module Random
    __bind_static_method__ :random_bytes, :OpenSSL_Random_random_bytes
  end

  class Digest
    attr_reader :name

    def self.digest(name, data)
      new(name).digest(data)
    end

    def self.base64digest(name, data)
      new(name).base64digest(data)
    end

    def self.hexdigest(name, data)
      new(name).hexdigest(data)
    end

    def base64digest(data)
      [digest(data)].pack('m0')
    end

    def hexdigest(data)
      digest(data).unpack1('H*')
    end

    class SHA1 < Digest
      def initialize()
        super('SHA1')
      end
    end

    class SHA256 < Digest
      def initialize()
        super('SHA256')
      end
    end

    class SHA384 < Digest
      def initialize()
        super('SHA384')
      end
    end

    class SHA512 < Digest
      def initialize()
        super('SHA512')
      end
    end
  end
end
