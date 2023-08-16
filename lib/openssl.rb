require 'natalie/inline'
require 'openssl.cpp'

__ld_flags__ '-lcrypto'

module OpenSSL
  module Random
    __bind_static_method__ :random_bytes, :OpenSSL_Random_random_bytes
  end

  class Digest
    def self.digest(name, data)
      klass = const_get(name.to_s.upcase.to_sym)
      raise NameError unless klass.ancestors[1] == self
      klass.new.digest(data)
    rescue NameError
      raise "Unsupported digest algorithm (#{name}).: unknown object name"
    end

    def self.base64digest(name, data)
      [digest(name, data)].pack('m0')
    end

    def self.hexdigest(name, data)
      digest(name, data).unpack1('H*')
    end

    class SHA1 < Digest
      __bind_method__ :digest, :OpenSSL_Digest_SHA1_digest
    end

    class SHA256 < Digest
      __bind_method__ :digest, :OpenSSL_Digest_SHA256_digest
    end

    class SHA384 < Digest
      __bind_method__ :digest, :OpenSSL_Digest_SHA384_digest
    end

    class SHA512 < Digest
      __bind_method__ :digest, :OpenSSL_Digest_SHA512_digest
    end
  end
end
