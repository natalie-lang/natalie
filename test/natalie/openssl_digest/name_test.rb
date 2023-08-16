require_relative '../../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::Digest#name" do
  it "returns the name of digest when called on an instance" do
    NATFIXME 'Implement OpenSSL::Digest#name', exception: NoMethodError, message: "undefined method `name'" do
      OpenSSL::Digest.new('SHA1').name.should == 'SHA1'
    end
  end

  it "converts the name to the internal representation of OpenSSL" do
    NATFIXME 'Implement OpenSSL::Digest#name', exception: NoMethodError, message: "undefined method `name'" do
      OpenSSL::Digest.new('sha1').name.should == 'SHA1'
    end
  end

  it "works on subclasses too" do
    NATFIXME 'Implement OpenSSL::Digest#name', exception: NoMethodError, message: "undefined method `name'" do
      OpenSSL::Digest::SHA1.new.name.should == 'SHA1'
    end
  end
end
