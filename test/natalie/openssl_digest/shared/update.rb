require_relative '../../../../spec/library/digest/sha1/shared/constants'
require_relative '../../../../spec/library/digest/sha256/shared/constants'
require_relative '../../../../spec/library/digest/sha384/shared/constants'
require_relative '../../../../spec/library/digest/sha512/shared/constants'
require 'openssl'

describe :openssl_digest_update, shared: true do
  context "it supports full data chunks" do
    it "returns a SHA1 digest" do
      digest = OpenSSL::Digest.new('sha1')
      digest.send(@method, SHA1Constants::Contents)
      digest.digest.should == SHA1Constants::Digest
    end

    it "returns a SHA256 digest" do
      digest = OpenSSL::Digest.new('sha256')
      digest.send(@method, SHA256Constants::Contents)
      digest.digest.should == SHA256Constants::Digest
    end

    it "returns a SHA384 digest" do
      digest = OpenSSL::Digest.new('sha384')
      digest.send(@method, SHA384Constants::Contents)
      digest.digest.should == SHA384Constants::Digest
    end

    it "returns a SHA512 digest" do
      digest = OpenSSL::Digest.new('sha512')
      digest.send(@method, SHA512Constants::Contents)
      digest.digest.should == SHA512Constants::Digest
    end
  end

  context "it support smaller chunks" do
    it "returns a SHA1 digest" do
      digest = OpenSSL::Digest.new('sha1')
      SHA1Constants::Contents.each_char { |b| digest.send(@method, b) }
      digest.digest.should == SHA1Constants::Digest
    end

    it "returns a SHA256 digest" do
      digest = OpenSSL::Digest.new('sha256')
      SHA256Constants::Contents.each_char { |b| digest.send(@method, b) }
      digest.digest.should == SHA256Constants::Digest
    end

    it "returns a SHA384 digest" do
      digest = OpenSSL::Digest.new('sha384')
      SHA384Constants::Contents.each_char { |b| digest.send(@method, b) }
      digest.digest.should == SHA384Constants::Digest
    end

    it "returns a SHA512 digest" do
      digest = OpenSSL::Digest.new('sha512')
      SHA512Constants::Contents.each_char { |b| digest.send(@method, b) }
      digest.digest.should == SHA512Constants::Digest
    end
  end
end
