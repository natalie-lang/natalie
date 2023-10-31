require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher.new" do
  it "can be created with 'aes-256-cbc'" do
    OpenSSL::Cipher.new("aes-256-cbc").should be_kind_of(OpenSSL::Cipher)
  end

  it "can be created with 'AES-256-CBC'" do
    OpenSSL::Cipher.new("AES-256-CBC").should be_kind_of(OpenSSL::Cipher)
  end

  it "coerces the name to a String using #to_str" do
    name = mock("name")
    name.should_receive(:to_str).and_return("aes-256-cbc")
    OpenSSL::Cipher.new(name).should be_kind_of(OpenSSL::Cipher)
  end

  it "raises a RuntimeError when created with a non valid cipher name" do
    -> {
      OpenSSL::Cipher.new("sea-652-cbc")
    }.should raise_error(RuntimeError, "unsupported cipher algorithm (sea-652-cbc)")
  end

  it "raises a TypeError when the name is not a String and canont be coerced into a String" do
    -> {
      OpenSSL::Cipher.new(Object.new)
    }.should raise_error(TypeError, "no implicit conversion of Object into String")
  end

  it "raises an ArgumentError when not created with a cipher name" do
    -> {
      OpenSSL::Cipher.new
    }.should raise_error(ArgumentError, "wrong number of arguments (given 0, expected 1)")

    -> {
      OpenSSL::Cipher.new("aes-256-cbc", "plaintext")
    }.should raise_error(ArgumentError, "wrong number of arguments (given 2, expected 1)")
  end
end
