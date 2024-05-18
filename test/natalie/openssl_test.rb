require_relative '../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::OPENSSL_VERSION_NUMBER" do
  it "is a number" do
    OpenSSL::OPENSSL_VERSION_NUMBER.should be_kind_of(Integer)
  end
end

describe "OpenSSL::OPENSSL_VERSION" do
  it "is a string" do
    OpenSSL::OPENSSL_VERSION.should be_kind_of(String)
  end
end

describe "OpenSSL::BN" do
  describe "OpenSSL::BN.new" do
    it "can be constructed with an Integer" do
      bn = OpenSSL::BN.new(1234)
      bn.should be_kind_of(OpenSSL::BN)

      bn = OpenSSL::BN.new(1 << 30) # Natalie Bignum type
      bn.should be_kind_of(OpenSSL::BN)
    end

    it "can be constructor with a different BN object" do
      bn = OpenSSL::BN.new(1234)
      bn.should == OpenSSL::BN.new(bn)
    end

    it "can be constructed with a String" do
      bn = OpenSSL::BN.new("1234")
      bn.should be_kind_of(OpenSSL::BN)
      bn.to_i.should == 1234
    end

    it "can be constructed with a String with a base" do
      bn = OpenSSL::BN.new("1234", 16)
      bn.should be_kind_of(OpenSSL::BN)
      bn.to_i.should == 0x1234
    end
  end

  describe "OpenSSL::BN#<=>" do
    it "can compare two OpenSSL::BN instances" do
      OpenSSL::BN.new(1234).should == OpenSSL::BN.new(1234)
      OpenSSL::BN.new(1234).should != OpenSSL::BN.new(4321)
    end

    it "can compare a OpenSSL::BN instance to an Integer" do
      OpenSSL::BN.new(1234).should == 1234
      OpenSSL::BN.new(1234).should != 4321
    end

    it "can compare a OpenSSL::BN instance to a Natalie Bignum type" do
      OpenSSL::BN.new(1 << 30).should == 1 << 30
    end
  end

  describe "OpenSSL::BN#to_i" do
    it "returns the value as an Integer" do
      bn = OpenSSL::BN.new(1234)
      bn.to_i.should == 1234
    end

    it "supports the Natalie Bignum type" do
      bn = OpenSSL::BN.new(1 << 30)
      bn.to_i.should == 1 << 30
    end
  end
end

describe "OpenSSL::X509::Certificate" do
  describe "OpenSSL::X509::Certificate#issuer" do
    it "can be set and queried with OpenSSL::X509::Name" do
      cert = OpenSSL::X509::Certificate.new
      cert.issuer = OpenSSL::X509::Name.parse("/DC=org/DC=truffleruby/CN=TruffleRuby CA")
      cert.issuer.should be_kind_of(OpenSSL::X509::Name)
      cert.issuer.should == OpenSSL::X509::Name.parse("/DC=org/DC=truffleruby/CN=TruffleRuby CA")
    end

    it "has a default issuer" do
      cert = OpenSSL::X509::Certificate.new
      cert.issuer.should be_kind_of(OpenSSL::X509::Name)
      cert.issuer.should == OpenSSL::X509::Name.new
    end

    it "cannot be set with String" do
      cert = OpenSSL::X509::Certificate.new
      -> {
        cert.issuer = "/DC=org/DC=truffleruby/CN=TruffleRuby CA"
      }.should raise_error(TypeError, "wrong argument type String (expected OpenSSL/X509/NAME)")
    end
  end

  describe "OpenSSL::X509::Certificate#not_after" do
    before :each do
      # Time compare with different timezones is broken in Natalie, so run this in UTC.
      # The following code yields `true` in MRI, but `false` in Natalie:
      #
      #     Time.new(2024, 1, 1, 0, 0, 0, 3600) == Time.new(2023, 12, 31, 23, 0, 0, 0)
      @tz, ENV["TZ"] = ENV["TZ"], "UTC"
    end

    after :each do
      ENV["TZ"] = @tz
    end

    it "can be set and queried with Time" do
      time = Time.now + 10
      cert = OpenSSL::X509::Certificate.new
      cert.not_after = time
      cert.not_after.should > time - 1
      cert.not_after.should < time + 1
    end

    it "can be set with Integer" do
      time = Time.now + 10
      cert = OpenSSL::X509::Certificate.new
      cert.not_after = time.to_i
      cert.not_after.should > time - 1
      cert.not_after.should < time + 1
    end

    it "returns time in UTC" do
      ENV["TZ"] = "CET"
      time = Time.new(2024, 1, 1, 0, 0, 0, 3600)
      cert = OpenSSL::X509::Certificate.new
      cert.not_after = time
      cert.not_after.should == Time.new(2023, 12, 31, 23, 0, 0, 0)
    end

    it "has no default value" do
      cert = OpenSSL::X509::Certificate.new
      cert.not_after.should be_nil
    end

    it "cannot be set with String" do
      cert = OpenSSL::X509::Certificate.new
      -> {
        cert.not_after = "2999-01-01 00:00:00"
      }.should raise_error(ArgumentError, /invalid value for Integer\(\):/)
    end
  end

  describe "OpenSSL::X509::Certificate#not_before" do
    before :each do
      # Time compare with different timezones is broken in Natalie, so run this in UTC.
      # The following code yields `true` in MRI, but `false` in Natalie:
      #
      #     Time.new(2024, 1, 1, 0, 0, 0, 3600) == Time.new(2023, 12, 31, 23, 0, 0, 0)
      @tz, ENV["TZ"] = ENV["TZ"], "UTC"
    end

    after :each do
      ENV["TZ"] = @tz
    end

    it "can be set and queried with Time" do
      time = Time.now - 10
      cert = OpenSSL::X509::Certificate.new
      cert.not_before = time
      cert.not_before.should > time - 1
      cert.not_before.should < time + 1
    end

    it "can be set with Integer" do
      time = Time.now - 10
      cert = OpenSSL::X509::Certificate.new
      cert.not_before = time.to_i
      cert.not_before.should > time - 1
      cert.not_before.should < time + 1
    end

    it "returns time in UTC" do
      ENV["TZ"] = "CET"
      time = Time.new(2024, 1, 1, 0, 0, 0, 3600)
      cert = OpenSSL::X509::Certificate.new
      cert.not_before = time
      cert.not_before.should == Time.new(2023, 12, 31, 23, 0, 0, 0)
    end

    it "has no default value" do
      cert = OpenSSL::X509::Certificate.new
      cert.not_before.should be_nil
    end

    it "cannot be set with String" do
      cert = OpenSSL::X509::Certificate.new
      -> {
        cert.not_before = "2999-01-01 00:00:00"
      }.should raise_error(ArgumentError, /invalid value for Integer\(\):/)
    end
  end

  describe "OpenSSL::X509::Certificate#public_key" do
    it "can be set and queried with OpenSSL::PKey::RSA" do
      key = OpenSSL::PKey::RSA.new(2048)
      cert = OpenSSL::X509::Certificate.new
      cert.public_key = key.public_key
      cert.public_key.export.should == key.public_key.export
    end

    it "has no default value" do
      cert = OpenSSL::X509::Certificate.new
      -> { cert.public_key }.should raise_error(OpenSSL::X509::CertificateError)
    end

    it "cannot be set with String" do
      key = OpenSSL::PKey::RSA.new(2048)
      cert = OpenSSL::X509::Certificate.new
      -> {
        cert.public_key = key.public_key.to_s
      }.should raise_error(TypeError, "wrong argument type String (expected OpenSSL/EVP_PKEY)")
    end
  end

  describe "OpenSSL::X509::Certificate#serial" do
    it "can be set and queried with integer" do
      cert = OpenSSL::X509::Certificate.new
      cert.serial = 2
      cert.serial.should == 2
    end

    it "can be set and queried with OpenSSL::BN" do
      cert = OpenSSL::X509::Certificate.new
      cert.serial = OpenSSL::BN.new(2)
      cert.serial.should == OpenSSL::BN.new(2)
    end

    it "has a default serial" do
      cert = OpenSSL::X509::Certificate.new
      # Default serial might depend on OpenSSL settings/version, so just check the type
      cert.serial.should be_kind_of(OpenSSL::BN)
    end

    it "does not require the serial to be positive" do
      cert = OpenSSL::X509::Certificate.new
      -> { cert.serial = -1 }.should_not raise_error
    end

    it "raises a TypeError on invalid input type" do
      cert = OpenSSL::X509::Certificate.new
      -> { cert.serial = Object.new }.should raise_error(TypeError, 'Cannot convert into OpenSSL::BN')
    end

    it "does not coerce the input with #to_int" do
      serial = mock("version")
      serial.should_not_receive(:to_int)
      cert = OpenSSL::X509::Certificate.new
      -> { cert.serial = serial }.should raise_error(TypeError, 'Cannot convert into OpenSSL::BN')
    end
  end

  describe "OpenSSL::X509::Certificate#sign" do
    it "can can sign using an RSA key (after settings tons of options)" do
      key = OpenSSL::PKey::RSA.new(2048)
      cert = OpenSSL::X509::Certificate.new
      cert.version = 2
      cert.serial = 1
      cert.subject = OpenSSL::X509::Name.parse "/DC=org/DC=truffleruby/CN=TruffleRuby CA"
      cert.issuer = cert.subject
      cert.public_key = key.public_key
      cert.not_before = Time.now - 10
      cert.not_after = cert.not_before + 365 * 24 * 60 * 60
      cert.sign(key, OpenSSL::Digest.new("SHA256"))
    end

    it "can convert a String digest argument into a Digest class" do
      key = OpenSSL::PKey::RSA.new(2048)
      cert = OpenSSL::X509::Certificate.new
      cert.version = 2
      cert.serial = 1
      cert.subject = OpenSSL::X509::Name.parse "/DC=org/DC=truffleruby/CN=TruffleRuby CA"
      cert.issuer = cert.subject
      cert.public_key = key.public_key
      cert.not_before = Time.now - 10
      cert.not_after = cert.not_before + 365 * 24 * 60 * 60
      cert.sign(key, "SHA256")
    end

    it "raises a CertificateError when required settings are missing" do
      key = OpenSSL::PKey::RSA.new(2048)
      cert = OpenSSL::X509::Certificate.new
      -> {
        cert.sign(key, OpenSSL::Digest.new("SHA256"))
      }.should raise_error(OpenSSL::X509::CertificateError, "internal error")
    end

    it "does not convert a PEM key export into a PKey" do
      key = OpenSSL::PKey::RSA.new(2048)
      cert = OpenSSL::X509::Certificate.new
      -> {
        cert.sign(key.export, OpenSSL::Digest.new("SHA256"))
      }.should raise_error(TypeError, "wrong argument type String (expected OpenSSL/EVP_PKEY)")
    end

    it "cannot be signed with a public key" do
      key = OpenSSL::PKey::RSA.new(2048)
      cert = OpenSSL::X509::Certificate.new
      cert.version = 2
      cert.serial = 1
      cert.subject = OpenSSL::X509::Name.parse "/DC=org/DC=truffleruby/CN=TruffleRuby CA"
      cert.issuer = cert.subject
      cert.public_key = key.public_key
      cert.not_before = Time.now - 10
      cert.not_after = cert.not_before + 365 * 24 * 60 * 60
      -> {
        cert.sign(key.public_key, OpenSSL::Digest.new("SHA256"))
      }.should raise_error(ArgumentError, "private key is needed")
    end

    it "does not convert a Symbol into a Digest class" do
      key = OpenSSL::PKey::RSA.new(2048)
      cert = OpenSSL::X509::Certificate.new
      -> {
        cert.sign(key, :SHA256)
      }.should raise_error(TypeError, "wrong argument type Symbol (expected OpenSSL/Digest)")
    end
  end

  describe "OpenSSL::X509::Certificate#subject" do
    it "can be set and queried with OpenSSL::X509::Name" do
      cert = OpenSSL::X509::Certificate.new
      cert.subject = OpenSSL::X509::Name.parse("/DC=org/DC=truffleruby/CN=TruffleRuby CA")
      cert.subject.should be_kind_of(OpenSSL::X509::Name)
      cert.subject.should == OpenSSL::X509::Name.parse("/DC=org/DC=truffleruby/CN=TruffleRuby CA")
    end

    it "has a default subject" do
      cert = OpenSSL::X509::Certificate.new
      cert.subject.should be_kind_of(OpenSSL::X509::Name)
      cert.subject.should == OpenSSL::X509::Name.new
    end

    it "cannot be set with String" do
      cert = OpenSSL::X509::Certificate.new
      -> {
        cert.subject = "/DC=org/DC=truffleruby/CN=TruffleRuby CA"
      }.should raise_error(TypeError, "wrong argument type String (expected OpenSSL/X509/NAME)")
    end
  end

  describe "OpenSSL::X509::Certificate#version" do
    it "can be set and queried" do
      cert = OpenSSL::X509::Certificate.new
      cert.version = 2
      cert.version.should == 2
    end

    it "has a default version" do
      cert = OpenSSL::X509::Certificate.new
      # Default version might depend on OpenSSL settings/version, so just check the type
      cert.version.should be_kind_of(Integer)
    end

    it "coerces the input with #to_int" do
      version = mock("version")
      version.should_receive(:to_int).and_return(1)
      cert = OpenSSL::X509::Certificate.new
      cert.version = version
      cert.version.should == 1
    end

    it "raises a TypeError on invalid input type" do
      cert = OpenSSL::X509::Certificate.new
      -> { cert.version = Object.new }.should raise_error(TypeError, "no implicit conversion of Object into Integer")
    end

    it "requires the version to be positive" do
      cert = OpenSSL::X509::Certificate.new
      -> { cert.version = -1 }.should raise_error(OpenSSL::X509::CertificateError, "version must be >= 0!")
    end
  end
end

describe "OpenSSL::X509::Name" do
  describe "OpenSSL::X509::Name#initialize" do
    it "can handle input with types" do
      input = [
        ["DC", "org", OpenSSL::ASN1::IA5STRING],
        ["DC", "ruby-lang", OpenSSL::ASN1::IA5STRING],
        ["CN", "www.ruby-lang.org", OpenSSL::ASN1::UTF8STRING]
      ]
      name = OpenSSL::X509::Name.new(input)

      ary = name.to_a

      ary[0][0].should == "DC"
      ary[1][0].should == "DC"
      ary[2][0].should == "CN"
      ary[0][1].should == "org"
      ary[1][1].should == "ruby-lang"
      ary[2][1].should == "www.ruby-lang.org"
      ary[0][2].should == OpenSSL::ASN1::IA5STRING
      ary[1][2].should == OpenSSL::ASN1::IA5STRING
      ary[2][2].should == OpenSSL::ASN1::UTF8STRING
    end

    it "can handle input without types" do
      input = [
        ["DC", "org"],
        ["DC", "ruby-lang"],
        ["CN", "www.ruby-lang.org"]
      ]
      name = OpenSSL::X509::Name.new(input)

      ary = name.to_a

      ary[0][0].should == "DC"
      ary[1][0].should == "DC"
      ary[2][0].should == "CN"
      ary[0][1].should == "org"
      ary[1][1].should == "ruby-lang"
      ary[2][1].should == "www.ruby-lang.org"
      ary[0][2].should == OpenSSL::ASN1::IA5STRING
      ary[1][2].should == OpenSSL::ASN1::IA5STRING
      ary[2][2].should == OpenSSL::ASN1::UTF8STRING
    end

    it "can use a custom template" do
      template = { "DC" => OpenSSL::ASN1::UTF8STRING}
      template.default = OpenSSL::ASN1::IA5STRING
      input = [
        ["DC", "org"],
        ["DC", "ruby-lang"],
        ["CN", "www.ruby-lang.org"]
      ]
      name = OpenSSL::X509::Name.new(input, template)

      ary = name.to_a

      ary[0][0].should == "DC"
      ary[1][0].should == "DC"
      ary[2][0].should == "CN"
      ary[0][1].should == "org"
      ary[1][1].should == "ruby-lang"
      ary[2][1].should == "www.ruby-lang.org"
      ary[0][2].should == OpenSSL::ASN1::UTF8STRING
      ary[1][2].should == OpenSSL::ASN1::UTF8STRING
      ary[2][2].should == OpenSSL::ASN1::IA5STRING
    end
  end

  describe "OpenSSL::X509::Name#to_s" do
    it "returns the correct string" do
      name = OpenSSL::X509::Name.new
      name.add_entry("DC", "org")
      name.add_entry("DC", "ruby-lang")
      name.add_entry("CN", "www.ruby-lang.org")

      name.to_s.should == "/DC=org/DC=ruby-lang/CN=www.ruby-lang.org"
      name.to_s(OpenSSL::X509::Name::COMPAT).should == "DC=org, DC=ruby-lang, CN=www.ruby-lang.org"
      name.to_s(OpenSSL::X509::Name::RFC2253).should == "CN=www.ruby-lang.org,DC=ruby-lang,DC=org"
      name.to_s(OpenSSL::X509::Name::ONELINE).should == "DC = org, DC = ruby-lang, CN = www.ruby-lang.org"
      name.to_s(OpenSSL::X509::Name::MULTILINE).should == <<~NAME.chomp
        domainComponent           = org
        domainComponent           = ruby-lang
        commonName                = www.ruby-lang.org
      NAME
    end
  end
end

describe "OpenSSL::X509::Store" do
  describe ".new" do
    it "can be constructed without arguments" do
      store = OpenSSL::X509::Store.new
      store.should be_kind_of(OpenSSL::X509::Store)
    end
  end

  describe "#add_cert" do
    it "can be called with a OpenSSL::X509::Certificate" do
      cert = OpenSSL::X509::Certificate.new
      store = OpenSSL::X509::Store.new
      store.add_cert(cert).should == store
    end

    it "cannot be called with other objects" do
      store = OpenSSL::X509::Store.new
      -> { store.add_cert("cert") }.should raise_error(TypeError, "wrong argument type String (expected OpenSSL/X509)")
    end
  end

  describe "#set_default_paths" do
    it "can be called" do
      store = OpenSSL::X509::Store.new
      store.set_default_paths.should be_nil
    end
  end
end

describe "OpenSSL::SSL::SSLContext" do
  describe ".new" do
    it "can be constructed without arguments" do
      context = OpenSSL::SSL::SSLContext.new
      context.should be_kind_of(OpenSSL::SSL::SSLContext)
    end

    # NATFIXME: It can be constructed with 1 argument too, but that is deprecated, do we want to reproduce that?
  end

  describe "#cert_store" do
    it "can be set using a Store" do
      cert_store = OpenSSL::X509::Store.new
      context = OpenSSL::SSL::SSLContext.new
      context.cert_store = cert_store
      context.cert_store.should be_kind_of(OpenSSL::X509::Store)
    end

    it "returns the same object" do
      cert_store = OpenSSL::X509::Store.new
      context = OpenSSL::SSL::SSLContext.new
      context.cert_store = cert_store
      context.cert_store.should equal(cert_store)
    end

    it "saves the Store as an instance variable" do
      cert_store = OpenSSL::X509::Store.new
      context = OpenSSL::SSL::SSLContext.new
      context.instance_variables.should_not.include?(:@cert_store)
      context.cert_store = cert_store
      context.instance_variables.should.include?(:@cert_store)
    end

    it "can be set to any type (It raises an exception when calling setup)" do
      cert_store = Object.new
      context = OpenSSL::SSL::SSLContext.new
      context.cert_store = cert_store
      context.cert_store.should == cert_store
    end
  end

  describe "#max_version=" do
    it "can be set using a constant" do
      context = OpenSSL::SSL::SSLContext.new
      context.max_version = OpenSSL::SSL::TLS1_3_VERSION
    end

    it "can be set using a Symbol" do
      context = OpenSSL::SSL::SSLContext.new
      context.max_version = :TLS1_3
    end

    it "can be set using a String" do
      context = OpenSSL::SSL::SSLContext.new
      context.max_version = "TLS1_3"
    end

    it "cannot be set to a wrong numeric value" do
      context = OpenSSL::SSL::SSLContext.new
      -> { context.max_version = 1 }.should raise_error(OpenSSL::SSL::SSLError, /SSL_CTX_set_max_proto_version/)
    end

    it "cannot be set to a wrong Symbol" do
      context = OpenSSL::SSL::SSLContext.new
      -> { context.max_version = :MD5 }.should raise_error(ArgumentError, 'unrecognized version "MD5"')
    end

    it "cannot be set using the constant name as a Symbol" do
      context = OpenSSL::SSL::SSLContext.new
      -> { context.max_version = :TLS1_3_VERSION }.should raise_error(ArgumentError, 'unrecognized version "TLS1_3_VERSION"')
    end
  end

  describe "#min_version=" do
    it "can be set using a constant" do
      context = OpenSSL::SSL::SSLContext.new
      context.min_version = OpenSSL::SSL::TLS1_3_VERSION
    end

    it "can be set using a Symbol" do
      context = OpenSSL::SSL::SSLContext.new
      context.min_version = :TLS1_3
    end

    it "can be set using a String" do
      context = OpenSSL::SSL::SSLContext.new
      context.min_version = "TLS1_3"
    end

    it "cannot be set to a wrong numeric value" do
      context = OpenSSL::SSL::SSLContext.new
      -> { context.min_version = 1 }.should raise_error(OpenSSL::SSL::SSLError, /SSL_CTX_set_min_proto_version/)
    end

    it "cannot be set to a wrong Symbol" do
      context = OpenSSL::SSL::SSLContext.new
      -> { context.min_version = :MD5 }.should raise_error(ArgumentError, 'unrecognized version "MD5"')
    end

    it "cannot be set using the constant name as a Symbol" do
      context = OpenSSL::SSL::SSLContext.new
      -> { context.min_version = :TLS1_3_VERSION }.should raise_error(ArgumentError, 'unrecognized version "TLS1_3_VERSION"')
    end

    it "can be set to versions conflicting with max" do
      context = OpenSSL::SSL::SSLContext.new
      context.max_version = OpenSSL::SSL::TLS1_2_VERSION
      context.min_version = OpenSSL::SSL::TLS1_3_VERSION
    end
  end

  describe "#options" do
    it "has a default value" do
      context = OpenSSL::SSL::SSLContext.new
      context.options.should == OpenSSL::SSL::OP_ALL | OpenSSL::SSL::OP_NO_COMPRESSION | OpenSSL::SSL::OP_ENABLE_MIDDLEBOX_COMPAT
    end

    it "can be set with an integer" do
      context = OpenSSL::SSL::SSLContext.new
      context.options = 1
      context.options.should == 1
    end

    it "converts the input with #to_int" do
      options = mock("options")
      options.should_receive(:to_int).and_return(2)
      context = OpenSSL::SSL::SSLContext.new
      context.options = options
      context.options.should == 2
    end

    it "overwrites the old options" do
      context = OpenSSL::SSL::SSLContext.new
      context.options = OpenSSL::SSL::OP_NO_SSLv3
      context.options.should == OpenSSL::SSL::OP_NO_SSLv3
    end

    it "can be reset with nil" do
      context = OpenSSL::SSL::SSLContext.new
      context.options = nil
      context.options.should == OpenSSL::SSL::OP_ALL
    end

    it "raises a TypeError on invalid input type" do
      context = OpenSSL::SSL::SSLContext.new
      -> {
        context.options = Object.new
      }.should raise_error(TypeError, "no implicit conversion of Object into Integer")
    end
  end

  describe "#security_level" do
    it "has a default value" do
      context = OpenSSL::SSL::SSLContext.new
      context.security_level.should be_kind_of(Integer)
    end

    it "can be set to an integer" do
      context = OpenSSL::SSL::SSLContext.new
      context.security_level = 2
      context.security_level.should == 2
    end

    it "converts the input with #to_int" do
      security_level = mock("security level")
      security_level.should_receive(:to_int).and_return(2)
      context = OpenSSL::SSL::SSLContext.new
      context.security_level = security_level
      context.security_level.should == 2
    end

    it "can be set to any integer value" do
      context = OpenSSL::SSL::SSLContext.new
      context.security_level = -2
      context.security_level.should == -2
    end
  end

  describe "#session_cache_mode" do
    it "has a default numeric value (the exact value may depend on OpenSSL" do
      context = OpenSSL::SSL::SSLContext.new
      context.session_cache_mode.should be_kind_of(Integer)
    end

    it "can be set to an integer" do
      context = OpenSSL::SSL::SSLContext.new
      context.session_cache_mode = 1
      context.session_cache_mode.should == 1
    end

    it "can be set to 0" do
      context = OpenSSL::SSL::SSLContext.new
      context.session_cache_mode = 0
      context.session_cache_mode.should == 0
    end

    it "converts the input with #to_int" do
      session_cache_mode = mock("security level")
      session_cache_mode.should_receive(:to_int).and_return(2)
      context = OpenSSL::SSL::SSLContext.new
      context.session_cache_mode = session_cache_mode
      context.session_cache_mode.should == 2
    end

    it "cannot be set to nil" do
      context = OpenSSL::SSL::SSLContext.new
      -> {
        context.session_cache_mode = nil
      }.should raise_error(TypeError, "no implicit conversion from nil to integer")
    end

    it "raises a TypeError on invalid input type" do
      context = OpenSSL::SSL::SSLContext.new
      -> {
        context.session_cache_mode = Object.new
      }.should raise_error(TypeError, "no implicit conversion of Object into Integer")
    end
  end

  describe "#set_params" do
    it "sets the values of DEFAULT_PARAMS" do
      context = OpenSSL::SSL::SSLContext.new
      context.set_params
      # SSLContext#min_version does not exist
      context.verify_mode.should == OpenSSL::SSL::SSLContext::DEFAULT_PARAMS[:verify_mode]
      context.verify_hostname.should be_true
      context.options.should == OpenSSL::SSL::SSLContext::DEFAULT_PARAMS[:options]
    end

    it "merges the argument" do
      context = OpenSSL::SSL::SSLContext.new
      context.set_params({ verify_hostname: false })
      context.verify_mode.should == OpenSSL::SSL::SSLContext::DEFAULT_PARAMS[:verify_mode]
      context.verify_hostname.should be_false
    end

    it "returns the resulting hash with the options key removed" do
      params = { verify_hostname: false }
      context = OpenSSL::SSL::SSLContext.new
      result = context.set_params(params)
      result.should == OpenSSL::SSL::SSLContext::DEFAULT_PARAMS.merge(params).except(:options)
    end

    it "sets the cert_store to the default cert store" do
      context = OpenSSL::SSL::SSLContext.new
      context.set_params
      context.cert_store.should equal(OpenSSL::SSL::SSLContext::DEFAULT_CERT_STORE)
    end

    it "does not overwrite any previously set cert store" do
      cert_store = OpenSSL::X509::Store.new
      context = OpenSSL::SSL::SSLContext.new
      context.cert_store = cert_store
      context.set_params
      context.cert_store.should equal(cert_store)
    end
  end

  describe "#verify_mode" do
    it "defaults to 0" do
      context = OpenSSL::SSL::SSLContext.new
      context.verify_mode.should == 0
    end
  end

  describe "#verify_mode=" do
    it "can be called with any value without verification" do
      context = OpenSSL::SSL::SSLContext.new
      context.verify_mode = :foobar
    end
  end

  describe "#setup" do
    it "freezes the context" do
      context = OpenSSL::SSL::SSLContext.new
      context.should_not.frozen?

      context.setup
      context.should.frozen?
    end

    it "returns true the first time it is called" do
      context = OpenSSL::SSL::SSLContext.new
      context.setup.should be_true
    end

    it "returns nil the next time it is called" do
      context = OpenSSL::SSL::SSLContext.new
      context.setup
      context.setup.should be_nil
    end

    it "validates the verify mode" do
      context = OpenSSL::SSL::SSLContext.new
      context.verify_mode = :foobar
      -> { context.setup }.should raise_error(TypeError, "no implicit conversion of Symbol into Integer")
    end

    it "supports a nil value as verify mode to disable it" do
      context = OpenSSL::SSL::SSLContext.new
      context.verify_mode = nil
      -> { context.setup }.should_not raise_error
    end

    it "validates the cert_store object" do
      cert_store = Object.new
      context = OpenSSL::SSL::SSLContext.new
      context.cert_store = cert_store
      -> { context.setup }.should raise_error(TypeError, "wrong argument type Object (expected OpenSSL/X509/STORE)")
    end
  end
end

describe "OpenSSL::SSL::SSLSocket" do
  describe "#initialize" do
    it "can be constructed with a single IO object" do
      ssl_socket = OpenSSL::SSL::SSLSocket.new($stderr)
      ssl_socket.should be_kind_of(OpenSSL::SSL::SSLSocket)
      ssl_socket.io.should == $stderr
      ssl_socket.context.should be_kind_of(OpenSSL::SSL::SSLContext)
    end

    it "can be constructed with an IO object and an SSL context" do
      context = OpenSSL::SSL::SSLContext.new
      ssl_socket = OpenSSL::SSL::SSLSocket.new($stderr, context)
      ssl_socket.should be_kind_of(OpenSSL::SSL::SSLSocket)
      ssl_socket.io.should == $stderr
      ssl_socket.context.should == context
    end

    it "calls SSLContext#setup" do
      context = OpenSSL::SSL::SSLContext.new
      ssl_socket = OpenSSL::SSL::SSLSocket.new($stderr, context)
      context.setup.should be_nil
    end

    it "raises a TypeError if the first argument is not an IO object" do
      -> {
        OpenSSL::SSL::SSLSocket.new(42)
      }.should raise_error(TypeError, 'wrong argument type Integer (expected File)')
    end

    it "raises a TypeError if the second argument is not an SSLContext object" do
      -> {
        OpenSSL::SSL::SSLSocket.new($stderr, 42)
      }.should raise_error(TypeError, 'wrong argument type Integer (expected OpenSSL/SSL/CTX)')
    end
  end

  describe "#hostname=" do
    it "can be called with a String" do
      ssl_socket = OpenSSL::SSL::SSLSocket.new($stderr)
      ssl_socket.hostname = 'natalie-lang.org'
      ssl_socket.hostname.should == 'natalie-lang.org'
    end

    it "calls #to_str on input other than String" do
      mock = mock("to_str")
      mock.should_receive(:to_str).and_return('natalie-lang.org')
      ssl_socket = OpenSSL::SSL::SSLSocket.new($stderr)
      ssl_socket.hostname = mock
      ssl_socket.hostname.should == 'natalie-lang.org'
    end

    it "can clear a value with nil input" do
      ssl_socket = OpenSSL::SSL::SSLSocket.new($stderr)
      ssl_socket.hostname = 'natalie-lang.org'
      ssl_socket.hostname = nil
      ssl_socket.hostname.should be_nil
    end

    it "raises a TypeError for other input" do
      ssl_socket = OpenSSL::SSL::SSLSocket.new($stderr)
      -> {
        ssl_socket.hostname = 42
      }.should raise_error(TypeError, "no implicit conversion of Integer into String")
    end
  end
end

describe "OpenSSL::Digest.file" do
  it "can work with some argument shuffling on OpenSSL::Digest itself" do
    expect = OpenSSL::Digest.new('sha1').hexdigest(File.read(__FILE__))
    OpenSSL::Digest.file(__FILE__, 'sha1').hexdigest.should == expect
  end
end
