require_relative '../../../spec_helper'
require_relative '../shared/constants'
require 'openssl'

describe "OpenSSL::HMAC.hexdigest" do
  it "returns an SHA1 hex digest" do
    cur_digest = OpenSSL::Digest.new("SHA1")
    NATFIXME 'Implement OpenSSL::Digest#hexdigest with 0 arguments' do
      cur_digest.hexdigest.should == HMACConstants::BlankSHA1HexDigest
    end
    cur_digest.hexdigest('').should == HMACConstants::BlankSHA1HexDigest
    hexdigest = OpenSSL::HMAC.hexdigest(cur_digest,
                                        HMACConstants::Key,
                                        HMACConstants::Contents)
    hexdigest.should == HMACConstants::SHA1Hexdigest
  end
end

# Should add in similar specs for MD5, RIPEMD160, and SHA256
