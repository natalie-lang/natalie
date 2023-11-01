require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher#decrypt" do
  it "can be called" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.decrypt.should == cipher
  end

  it "can be called multiple times" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.decrypt
    cipher.decrypt.should == cipher
  end

  it "can be called after OpenSSL::Cipher#encrypt" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.encrypt
    cipher.decrypt.should == cipher
  end
end
