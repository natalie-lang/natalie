require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher#iv=" do
  it "can set an iv for aes-256-cbc" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.iv = "\x00".b * 16
  end

  it "can set an empty iv for aes-256-ecb" do
    cipher = OpenSSL::Cipher.new("aes-256-ecb")
    cipher.iv = "".b
  end

  it "coerces the iv into a String using #to_str" do
    iv = mock("iv")
    iv.should_receive(:to_str).and_return("\x00".b * 16)
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.iv = iv
  end

  it "cannot set an iv of an incorrect size" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    -> {
      cipher.iv = "\x00".b * 32
    }.should raise_error(ArgumentError, "iv must be 16 bytes")

    cipher = OpenSSL::Cipher.new("aes-256-ecb")
    -> {
      cipher.iv = "\x00".b * 32
    }.should raise_error(ArgumentError, "iv must be 0 bytes")
  end

  it "raises a TypeError if the iv is not a String and cannot be coerced into a String" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    -> {
      cipher.iv = 123
    }.should raise_error(TypeError, "no implicit conversion of Integer into String")
  end
end
