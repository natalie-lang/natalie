require_relative '../../../../spec_helper'
require 'openssl'

describe "OpenSSL::X509::Name.parse" do
  it "parses a /-delimited string of key-value pairs into a Name" do
    dn = "/DC=org/DC=ruby-lang/CN=www.ruby-lang.org"
    NATFIXME 'Implement OpenSSL::X509::Name.parse', exception: NoMethodError, message: "undefined method `parse' for OpenSSL::X509::Name:Class" do
      name = OpenSSL::X509::Name.parse(dn)
    end
    name = OpenSSL::X509::Name.new
    name.add_entry('DC', 'org')
    name.add_entry('DC', 'ruby-lang')
    name.add_entry('CN', 'www.ruby-lang.org')

    name.to_s.should == dn

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

  it "parses a comma-delimited string of key-value pairs into a name" do
    dn = "DC=org, DC=ruby-lang, CN=www.ruby-lang.org"
    NATFIXME 'Implement OpenSSL::X509::Name.parse', exception: NoMethodError, message: "undefined method `parse' for OpenSSL::X509::Name:Class" do
      name = OpenSSL::X509::Name.parse(dn)
    end
    name = OpenSSL::X509::Name.new
    name.add_entry('DC', 'org')
    name.add_entry('DC', 'ruby-lang')
    name.add_entry('CN', 'www.ruby-lang.org')

    name.to_s.should == "/DC=org/DC=ruby-lang/CN=www.ruby-lang.org"

    ary = name.to_a

    ary[0][1].should == "org"
    ary[1][1].should == "ruby-lang"
    ary[2][1].should == "www.ruby-lang.org"
  end

  it "raises TypeError if the given string contains no key/value pairs" do
    NATFIXME 'Implement OpenSSL::X509::Name.parse', exception: SpecFailedException do
      -> do
        OpenSSL::X509::Name.parse("hello")
      end.should raise_error(TypeError)
    end
  end

  it "raises OpenSSL::X509::NameError if the given string contains invalid keys" do
    NATFIXME 'Implement OpenSSL::X509::Name.parse', exception: SpecFailedException do
      -> do
        OpenSSL::X509::Name.parse("hello=goodbye")
      end.should raise_error(OpenSSL::X509::NameError)
    end
  end
end
