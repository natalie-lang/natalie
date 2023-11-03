require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher#key=" do
  it "can set a key for aes-256" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.key = "\x00".b * 32
  end

  it "can set a key for aes-128" do
    cipher = OpenSSL::Cipher.new("aes-128-cbc")
    cipher.key = "\x00".b * 16
  end

  it "coerces the key into a String using #to_str" do
    key = mock("key")
    key.should_receive(:to_str).and_return("\x00".b * 32)
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.key = key
  end

  it "cannot set a key of an incorrect size" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    -> {
      cipher.key = "\x00".b * 16
    }.should raise_error(ArgumentError, "key must be 32 bytes")

    cipher = OpenSSL::Cipher.new("aes-128-cbc")
    -> {
      cipher.key = "\x00".b * 32
    }.should raise_error(ArgumentError, "key must be 16 bytes")
  end

  it "raises a TypeError if the key is not a String and cannot be coerced into a String" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    -> {
      cipher.key = 123
    }.should raise_error(TypeError, "no implicit conversion of Integer into String")
  end
end
