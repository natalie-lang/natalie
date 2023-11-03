require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher#encrypt" do
  it "can be called" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.encrypt.should == cipher
  end

  it "can be called multiple times" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.encrypt
    cipher.encrypt.should == cipher
  end

  it "can be called after OpenSSL::Cipher#decrypt" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.decrypt
    cipher.encrypt.should == cipher
  end
end
