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

describe "OpenSSL::BN.new" do
  it "can be constructed with an integer in the int64 range" do
    bn = OpenSSL::BN.new(1234)
    bn.should be_kind_of(OpenSSL::BN)
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

describe "OpenSSL::SSL::SSLContext#initialize" do
  it "can be constructed without arguments" do
    context = OpenSSL::SSL::SSLContext.new
    context.should be_kind_of(OpenSSL::SSL::SSLContext)
  end

  # NATFIXME: It can be constructed with 1 argument too, but that is deprecated, do we want to reproduce that?
end

describe "OpenSSL::SSL::SSLSocket#initialize" do
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

  # NATFIXME: The macos CI runner detects the Ruby version as 3.0-, but the code behaves like 3.1+
  #           For now, just disable this test on Darwin, it does not add anything and the tests are
  #           mostly relevant for developing.
  platform_is_not :darwin do
    it "raises a TypeError if the first argument is not an IO object" do
      -> {
        OpenSSL::SSL::SSLSocket.new(42)
      }.should raise_error(TypeError, 'wrong argument type Integer (expected File)')
    end
  end

  it "raises a TypeError if the second argument is not an SSLContext object" do
    -> {
      OpenSSL::SSL::SSLSocket.new($stderr, 42)
    }.should raise_error(TypeError, 'wrong argument type Integer (expected OpenSSL/SSL/CTX)')
  end
end

describe "OpenSSL::Digest.file" do
  it "can work with some argument shuffling on OpenSSL::Digest itself" do
    expect = OpenSSL::Digest.new('sha1').hexdigest(File.read(__FILE__))
    OpenSSL::Digest.file(__FILE__, 'sha1').hexdigest.should == expect
  end
end
