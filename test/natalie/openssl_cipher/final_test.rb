require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Cipher#final" do
  it "can encrypt and decrypt data" do
    data = "Very, very confidential data"

    cipher = OpenSSL::Cipher.new('AES-128-CBC')
    cipher.encrypt
    key = cipher.random_key
    iv = cipher.random_iv
    encrypted = cipher.update(data) + cipher.final
    encrypted.should != data

    decipher = OpenSSL::Cipher.new('AES-128-CBC')
    decipher.decrypt
    decipher.key = key
    decipher.iv = iv
    plain = decipher.update(encrypted) + decipher.final
    plain.should == data
  end
end
