require 'natalie/inline'
require 'openssl.cpp'

__ld_flags__ '-lcrypto'
__ld_flags__ '-lssl'

# We have some circular dependencies here: our digest implementations are simply aliases to OpenSSL::Digest classes,
# but OpenSSL::Digest depends on ::Digest::Instance. So define this one in openssl.rb instead of digest.rb.
module Digest
  module Instance
    def update(_)
      raise "#{self} does not implement update()"
    end
    alias << update

    def new
      OpenSSL::Digest.new(name)
    end

    def ==(other)
      other = other.to_str if !other.is_a?(String) && other.respond_to?(:to_str)
      other = other.hexdigest if other.is_a?(self.class)
      hexdigest == other
    end

    def length
      digest_length
    end

    def size
      digest_length
    end

    def digest!(...)
      digest(...).tap { reset }
    end

    def hexdigest!(...)
      hexdigest(...).tap { reset }
    end

    def inspect
      return super if method(:update).owner == ::Digest::Instance
      "#<#{self.class}: #{hexdigest}>"
    end

    def to_s
      return super if method(:update).owner == ::Digest::Instance
      hexdigest
    end

    def self.included(klass)
      klass.define_singleton_method(:file) do |file, *args|
        file = file.to_str if !file.is_a?(String) && file.respond_to?(:to_str)
        raise TypeError, 'TODO' unless file.is_a?(String)
        new(*args, File.read(file))
      end
    end
  end
end

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

  class BN
    include Comparable

    __bind_method__ :initialize, :OpenSSL_BN_initialize
    __bind_method__ :<=>, :OpenSSL_BN_cmp
    __bind_method__ :to_i, :OpenSSL_BN_to_i

    alias to_int to_i
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
    include ::Digest::Instance
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
        define_singleton_method(:digest) { |*args| Digest.digest(normalized_name, *args) }
        define_singleton_method(:base64digest) { |*args| Digest.base64digest(normalized_name, *args) }
        define_singleton_method(:hexdigest) { |*args| Digest.hexdigest(normalized_name, *args) }
      end
      const_set(name, klass)
      klass
    rescue StandardError
      super
    end
  end

  class HMAC
    def self.hexdigest(digest, key, data)
      digest(digest, key, data).unpack1('H*')
    end
  end

  module SSL
    __constant__ 'SSL2_VERSION', 'int'
    __constant__ 'SSL3_VERSION', 'int'
    __constant__ 'TLS1_VERSION', 'int'
    __constant__ 'TLS1_1_VERSION', 'int'
    __constant__ 'TLS1_2_VERSION', 'int'
    __constant__ 'TLS1_3_VERSION', 'int'

    __constant__ 'VERIFY_NONE', 'int', 'SSL_VERIFY_NONE'
    __constant__ 'VERIFY_PEER', 'int', 'SSL_VERIFY_PEER'
    __constant__ 'VERIFY_FAIL_IF_NO_PEER_CERT', 'int', 'SSL_VERIFY_FAIL_IF_NO_PEER_CERT'
    __constant__ 'VERIFY_CLIENT_ONCE', 'int', 'SSL_VERIFY_CLIENT_ONCE'
    __constant__ 'VERIFY_POST_HANDSHAKE', 'int', 'SSL_VERIFY_POST_HANDSHAKE'

    class SSLError < OpenSSLError; end

    class SSLContext
      __bind_method__ :initialize, :OpenSSL_SSL_SSLContext_initialize
      __bind_method__ :max_version=, :OpenSSL_SSL_SSLContext_set_max_version
      __bind_method__ :min_version=, :OpenSSL_SSL_SSLContext_set_min_version
      __bind_method__ :security_level, :OpenSSL_SSL_SSLContext_security_level
      __bind_method__ :security_level=, :OpenSSL_SSL_SSLContext_set_security_level
      __bind_method__ :setup, :OpenSSL_SSL_SSLContext_setup
      __bind_method__ :verify_mode, :OpenSSL_SSL_SSLContext_verify_mode
      __bind_method__ :verify_mode=, :OpenSSL_SSL_SSLContext_set_verify_mode

      attr_accessor :cert_store

      alias freeze setup
    end

    class SSLSocket
      attr_reader :context, :io

      __bind_method__ :initialize, :OpenSSL_SSL_SSLSocket_initialize
      __bind_method__ :close, :OpenSSL_SSL_SSLSocket_close
      __bind_method__ :connect, :OpenSSL_SSL_SSLSocket_connect
      __bind_method__ :read, :OpenSSL_SSL_SSLSocket_read
      __bind_method__ :write, :OpenSSL_SSL_SSLSocket_write
    end
  end

  module KDF
    class KDFError < OpenSSLError; end
  end

  module PKey
    class PKeyError < OpenSSLError; end
    class RSAError < PKeyError; end

    class PKey; end

    class RSA < PKey
      __bind_method__ :initialize, :OpenSSL_PKey_RSA_initialize
      __bind_method__ :export, :OpenSSL_PKey_RSA_export
      __bind_method__ :private?, :OpenSSL_PKey_RSA_is_private
      __bind_method__ :public_key, :OpenSSL_PKey_RSA_public_key

      alias to_pem export
      alias to_s export

      def public?
        true
      end
    end
  end

  module X509
    class CertificateError < OpenSSLError; end
    class NameError < OpenSSLError; end
    class StoreError < OpenSSLError; end

    class Certificate
      __bind_method__ :initialize, :OpenSSL_X509_Certificate_initialize
      __bind_method__ :issuer, :OpenSSL_X509_Certificate_issuer
      __bind_method__ :issuer=, :OpenSSL_X509_Certificate_set_issuer
      __bind_method__ :not_after, :OpenSSL_X509_Certificate_not_after
      __bind_method__ :not_after=, :OpenSSL_X509_Certificate_set_not_after
      __bind_method__ :not_before, :OpenSSL_X509_Certificate_not_before
      __bind_method__ :not_before=, :OpenSSL_X509_Certificate_set_not_before
      __bind_method__ :public_key, :OpenSSL_X509_Certificate_public_key
      __bind_method__ :public_key=, :OpenSSL_X509_Certificate_set_public_key
      __bind_method__ :serial, :OpenSSL_X509_Certificate_serial
      __bind_method__ :serial=, :OpenSSL_X509_Certificate_set_serial
      __bind_method__ :sign, :OpenSSL_X509_Certificate_sign
      __bind_method__ :subject, :OpenSSL_X509_Certificate_subject
      __bind_method__ :subject=, :OpenSSL_X509_Certificate_set_subject
      __bind_method__ :version, :OpenSSL_X509_Certificate_version
      __bind_method__ :version=, :OpenSSL_X509_Certificate_set_version
    end

    class Name
      include Comparable

      OBJECT_TYPE_TEMPLATE = {
        'C'               => ASN1::PRINTABLESTRING,
        'countryName'     => ASN1::PRINTABLESTRING,
        'serialNumber'    => ASN1::PRINTABLESTRING,
        'dnQualifier'     => ASN1::PRINTABLESTRING,
        'DC'              => ASN1::IA5STRING,
        'domainComponent' => ASN1::IA5STRING,
        'emailAddress'    => ASN1::IA5STRING
      }.tap { |hash| hash.default = ASN1::UTF8STRING }.freeze

      __constant__('COMPAT', 'int', 'XN_FLAG_COMPAT')
      __constant__('RFC2253', 'int', 'XN_FLAG_RFC2253')
      __constant__('ONELINE', 'int', 'XN_FLAG_ONELINE')
      __constant__('MULTILINE', 'int', 'XN_FLAG_MULTILINE')

      __bind_method__ :initialize, :OpenSSL_X509_Name_initialize
      __bind_method__ :add_entry, :OpenSSL_X509_Name_add_entry
      __bind_method__ :to_a, :OpenSSL_X509_Name_to_a
      __bind_method__ :to_s, :OpenSSL_X509_Name_to_s
      __bind_method__ :<=>, :OpenSSL_X509_Name_cmp

      class << self
        def parse_openssl(str, template = OBJECT_TYPE_TEMPLATE)
          ary = if str.start_with?('/')
                  str.split('/')[1..]
                else
                  str.split(/,\s*/)
                end
          ary = ary.map { |e| e.split('=') }
          raise TypeError, 'invalid OpenSSL::X509::Name input' unless ary.all? { |e| e.size == 2 }
          new(ary, template)
        end

        alias parse parse_openssl
      end
    end

    class Store
      __bind_method__ :initialize, :OpenSSL_X509_Store_initialize, 0
      __bind_method__ :add_cert, :OpenSSL_X509_Store_add_cert, 1
      __bind_method__ :set_default_paths, :OpenSSL_X509_Store_set_default_paths, 0
      __bind_method__ :verify, :OpenSSL_X509_Store_verify

      attr_reader :error, :error_string
    end
  end
end
