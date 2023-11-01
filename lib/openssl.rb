require 'natalie/inline'
require 'openssl.cpp'

__ld_flags__ '-lcrypto'

module OpenSSL
  class OpenSSLError < StandardError; end

  def self.secure_compare(a, b)
    sha1_a = Digest.digest('SHA1', a)
    sha1_b = Digest.digest('SHA1', b)
    fixed_length_secure_compare(sha1_a, sha1_b) && a == b
  end

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

    def base64digest(...)
      [digest(...)].pack('m0')
    end

    def hexdigest(...)
      digest(...).unpack1('H*')
    end

    def self.const_missing(name)
      normalized_name = new(name.to_s).name
      raise if name.to_s != normalized_name
      klass = Class.new(self) do
        define_method(:initialize) { |*args| super(normalized_name, *args) }
      end
      const_set(name, klass)
      klass
    rescue
      super
    end
  end

  class HMAC
    def self.hexdigest(digest, key, data)
      digest(digest, key, data).unpack1('H*')
    end
  end

  module KDF
    class KDFError < OpenSSLError; end
  end
end
