require 'natalie/inline'
require 'openssl.cpp'

__ld_flags__ '-lcrypto'

module OpenSSL
  module Random
    __bind_static_method__ :random_bytes :OpenSSL_Random_random_bytes
  end

  class Digest
    def self.digest(digest, data)
      if digest.to_s.downcase == 'sha1'
        SHA1.new.digest(data)
      elsif digest.to_s.downcase == 'sha256'
        SHA256.new.digest(data)
      else
        raise NotImplementedError, "not implemented digest: #{digest}"
      end
    end

    def self.base64digest(digest, data)
      [digest(digest, data)].pack('m0')
    end

    def self.hexdigest(digest, data)
      digest(digest, data).unpack1('H*')
    end

    class SHA1
      __bind_method__ :digest, :OpenSSL_Digest_SHA1_digest
    end

    class SHA256
      __bind_method__ :digest, :OpenSSL_Digest_SHA256_digest
    end
  end
end
