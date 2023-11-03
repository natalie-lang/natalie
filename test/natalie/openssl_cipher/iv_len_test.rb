require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher#iv_len" do
  it "return the correct iv_len for aes-256" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.iv_len.should == 16
  end

  it "return 0 for ciphers without IV" do
    cipher = OpenSSL::Cipher.new("aes-256-ecb")
    cipher.iv_len.should == 0
  end
end
