require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher#random_iv" do
  it "can set an iv for aes-256-cbc" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    iv = cipher.random_iv
    iv.size.should == 16
    iv.encoding.should == Encoding::BINARY
  end

  it "generates an empty string for ciphers without an iv" do
    cipher = OpenSSL::Cipher.new("aes-256-ecb")
    iv = cipher.random_iv
    iv.should.empty?
    iv.encoding.should == Encoding::BINARY
  end

  it "should not generate the same iv twice" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    iv = cipher.random_iv
    cipher.random_iv.should != iv
  end
end
