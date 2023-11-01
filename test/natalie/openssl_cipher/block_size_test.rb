require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher#block_size" do
  it "return the correct blocksize for aes-256" do
    cipher = OpenSSL::Cipher.new("aes-256-cbc")
    cipher.block_size.should == 16
  end

  it "return 1 if the cipher does not use blocks" do
    cipher = OpenSSL::Cipher.new("chacha20")
    cipher.block_size.should == 1
  end
end
