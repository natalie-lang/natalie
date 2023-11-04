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

  module ASN1
    __constant__('EOC', 'int', 'V_ASN1_EOC')
    __constant__('BOOLEAN', 'int', 'V_ASN1_BOOLEAN')
    __constant__('INTEGER', 'int', 'V_ASN1_INTEGER')
    __constant__('BIT_STRING', 'int', 'V_ASN1_BIT_STRING')
    __constant__('OCTET_STRING', 'int', 'V_ASN1_OCTET_STRING')
    __constant__('NULL', 'int', 'V_ASN1_NULL')
    __constant__('OBJECT', 'int', 'V_ASN1_OBJECT')
    __constant__('OBJECT_DESCRIPTOR', 'int', 'V_ASN1_OBJECT_DESCRIPTOR')
    __constant__('EXTERNAL', 'int', 'V_ASN1_EXTERNAL')
    __constant__('REAL', 'int', 'V_ASN1_REAL')
    __constant__('ENUMERATED', 'int', 'V_ASN1_ENUMERATED')
    __constant__('UTF8STRING', 'int', 'V_ASN1_UTF8STRING')
    __constant__('SEQUENCE', 'int', 'V_ASN1_SEQUENCE')
    __constant__('SET', 'int', 'V_ASN1_SET')
    __constant__('NUMERICSTRING', 'int', 'V_ASN1_NUMERICSTRING')
    __constant__('PRINTABLESTRING', 'int', 'V_ASN1_PRINTABLESTRING')
    __constant__('T61STRING', 'int', 'V_ASN1_T61STRING')
    __constant__('VIDEOTEXSTRING', 'int', 'V_ASN1_VIDEOTEXSTRING')
    __constant__('IA5STRING', 'int', 'V_ASN1_IA5STRING')
    __constant__('UTCTIME', 'int', 'V_ASN1_UTCTIME')
    __constant__('GENERALIZEDTIME', 'int', 'V_ASN1_GENERALIZEDTIME')
    __constant__('GRAPHICSTRING', 'int', 'V_ASN1_GRAPHICSTRING')
    __constant__('ISO64STRING', 'int', 'V_ASN1_ISO64STRING')
    __constant__('GENERALSTRING', 'int', 'V_ASN1_GENERALSTRING')
    __constant__('UNIVERSALSTRING', 'int', 'V_ASN1_UNIVERSALSTRING')
    __constant__('BMPSTRING', 'int', 'V_ASN1_BMPSTRING')
  end

  module Random
    __bind_static_method__ :random_bytes, :OpenSSL_Random_random_bytes
  end

  class Cipher
    class CipherError < OpenSSLError; end

    def random_iv
      self.iv = Random.random_bytes(iv_len)
    end

    def random_key
      self.key = Random.random_bytes(key_len)
    end
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

  module X509
    class NameError < OpenSSLError; end

    class Name
      OBJECT_TYPE_TEMPLATE = {
        'C' => ASN1::PRINTABLESTRING,
        'countryName' => ASN1::PRINTABLESTRING,
        'serialNumber' => ASN1::PRINTABLESTRING,
        'dnQualifier' => ASN1::PRINTABLESTRING,
        'DC' => ASN1::IA5STRING,
        'domainComponent' => ASN1::IA5STRING,
        'emailAddress' => ASN1::IA5STRING
      }.tap { |hash| hash.default = ASN1::UTF8STRING }.freeze

      __bind_method__ :initialize, :OpenSSL_X509_Name_initialize
    end
  end
end
