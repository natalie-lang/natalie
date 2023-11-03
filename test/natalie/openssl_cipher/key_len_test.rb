require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher#key_len" do
  it "return the correct key_len for aes-256" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.key_len.should == 32
  end

  it "return the correct key_len for aes-128" do
    cipher = OpenSSL::Cipher.new("aes-128-cbc")
    cipher.key_len.should == 16
  end
end
