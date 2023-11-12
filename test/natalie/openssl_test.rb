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
    ruby_version_is ''...'3.1' do
      it "tries to call sync on the first argument" do
        -> {
          OpenSSL::SSL::SSLSocket.new(42)
        }.should raise_error(NoMethodError, "undefined method `sync' for 42:Integer")
      end
    end

    ruby_version_is '3.1' do
      it "raises a TypeError if the first argument is not an IO object" do
        -> {
          OpenSSL::SSL::SSLSocket.new(42)
        }.should raise_error(TypeError, 'wrong argument type Integer (expected File)')
      end
    end
  end

  it "raises a TypeError if the second argument is not an SSLContext object" do
    -> {
      OpenSSL::SSL::SSLSocket.new($stderr, 42)
    }.should raise_error(TypeError, 'wrong argument type Integer (expected OpenSSL/SSL/CTX)')
  end
end
