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

describe "OpenSSL.fixed_length_secure_compare" do
  it "returns true for two strings with the same content" do
    input1 = "the quick brown fox jumps over the lazy dog"
    input2 = "the quick brown fox jumps over the lazy dog"
    OpenSSL.fixed_length_secure_compare(input1, input2).should be_true
  end

  it "returns false for two strings of equal size with different content" do
    input1 = "the quick brown fox jumps over the lazy dog"
    input2 = "the lazy dog jumps over the quick brown fox"
    OpenSSL.fixed_length_secure_compare(input1, input2).should be_false
  end

  it "converts both arguments to strings using #to_str" do
    input1 = mock("input1")
    input1.should_receive(:to_str).and_return("the quick brown fox jumps over the lazy dog")
    input2 = mock("input2")
    input2.should_receive(:to_str).and_return("the quick brown fox jumps over the lazy dog")
    OpenSSL.fixed_length_secure_compare(input1, input2).should be_true
  end

  it "does not accept arguments that are not string and cannot be coerced into strings" do
    -> {
      OpenSSL.fixed_length_secure_compare("input1", :input2)
    }.should raise_error(TypeError, 'no implicit conversion of Symbol into String')

    -> {
      OpenSSL.fixed_length_secure_compare(Object.new, "input2")
    }.should raise_error(TypeError, 'no implicit conversion of Object into String')
  end

  it "raises an ArgumentError for two strings of different size" do
    input1 = "the quick brown fox jumps over the lazy dog"
    input2 = "the quick brown fox"
    -> {
      OpenSSL.fixed_length_secure_compare(input1, input2)
    }.should raise_error(ArgumentError, 'inputs must be of equal length')
  end
end

describe "OpenSSL.secure_compare" do
  it "returns true for two strings with the same content" do
    input1 = "the quick brown fox jumps over the lazy dog"
    input2 = "the quick brown fox jumps over the lazy dog"
    OpenSSL.secure_compare(input1, input2).should be_true
  end

  it "returns false for two strings with different content" do
    input1 = "the quick brown fox jumps over the lazy dog"
    input2 = "the lazy dog jumps over the quick brown fox"
    OpenSSL.secure_compare(input1, input2).should be_false
  end

  it "converts both arguments to strings using #to_str, but adds equality check for the original objects" do
    input1 = mock("input1")
    input1.should_receive(:to_str).and_return("the quick brown fox jumps over the lazy dog")
    input2 = mock("input2")
    input2.should_receive(:to_str).and_return("the quick brown fox jumps over the lazy dog")
    OpenSSL.secure_compare(input1, input2).should be_false

    input = mock("input")
    input.should_receive(:to_str).twice.and_return("the quick brown fox jumps over the lazy dog")
    OpenSSL.secure_compare(input, input).should be_true
  end

  it "does not accept arguments that are not string and cannot be coerced into strings" do
    -> {
      OpenSSL.secure_compare("input1", :input2)
    }.should raise_error(TypeError, 'no implicit conversion of Symbol into String')

    -> {
      OpenSSL.secure_compare(Object.new, "input2")
    }.should raise_error(TypeError, 'no implicit conversion of Object into String')
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
