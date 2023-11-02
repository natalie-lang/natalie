require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher#random_key" do
  it "can set a key for aes-256" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    key = cipher.random_key
    key.size.should == 32
    key.encoding.should == Encoding::BINARY
  end

  it "can set a key for aes-128" do
    cipher = OpenSSL::Cipher.new("aes-128-cbc")
    key = cipher.key = "\x00".b * 16
    key.size.should == 16
  end

  it "should not generate the same key twice" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    key = cipher.random_key
    cipher.random_key.should != key
  end
end
