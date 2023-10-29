require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher.ciphers" do
  it "returns an Array" do
    OpenSSL::Cipher.ciphers.should be_kind_of(Array)
  end

  it "is not empty" do
    OpenSSL::Cipher.ciphers.should_not.empty?
  end

  it "includes all the ciphers we use for testing" do
    OpenSSL::Cipher.ciphers.should.include?("aes-256-cbc")
    OpenSSL::Cipher.ciphers.should.include?("chacha20")
  end
end
