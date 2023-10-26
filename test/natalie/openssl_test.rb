require_relative '../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::OPENSSL_VERSION_NUMBER" do
  it "is a number" do
    OpenSSL::OPENSSL_VERSION_NUMBER.should be_kind_of(Integer)
  end
end

describe "OpenSSL::OPENSSL_VERSION" do
  it "is a string" do
    OpenSSL::OPENSSL_VERSION.should be_kind_of(String)
  end
end
