require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher#update" do
  it "returns an emtpy string when being called with a string smaller than block size" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.encrypt
    cipher.random_key
    cipher.random_iv
    cipher.block_size.should == 16
    cipher.update("." * 15).should == ""
  end

  it "returns a block sized string when being called with a string of at least 1 block size and less than 2 block sizes" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.encrypt
    cipher.random_key
    cipher.random_iv
    cipher.block_size.should == 16
    cipher.update("." * 31).size.should == 16
  end

  it "returns a binary string" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.encrypt
    cipher.random_key
    cipher.random_iv
    cipher.block_size.should == 16
    cipher.update("." * 31).encoding.should == Encoding::BINARY
  end

  it "coerces the input into a String using #to_str" do
    plaintext= mock("plaintext")
    plaintext.should_receive(:to_str).and_return("plaintext data")
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.encrypt
    cipher.random_key
    cipher.random_iv
    cipher.update(plaintext)
  end

  it "raises a TypeError if the data is not a String and cannot be coerced into a String" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.encrypt
    cipher.random_key
    cipher.random_iv
    -> {
      cipher.update(123)
    }.should raise_error(TypeError, "no implicit conversion of Integer into String")
  end

  it "can be called after decrypt too" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.decrypt
    cipher.random_key
    cipher.random_iv
    cipher.block_size.should == 16
    cipher.update("." * 15).should == ""
  end
end
