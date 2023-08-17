require_relative '../../../spec/spec_helper'
require_relative '../../../spec/library/digest/sha1/shared/constants'
require_relative '../../../spec/library/digest/sha256/shared/constants'
require_relative '../../../spec/library/digest/sha384/shared/constants'
require_relative '../../../spec/library/digest/sha512/shared/constants'
require 'openssl'

describe "OpenSSL::Digest initialization" do
  describe "can be called with a digest name" do
    it "returns a SHA1 object" do
      NATFIXME 'Implement OpenSSL::Digest#name', exception: NoMethodError, message: "undefined method `name'" do
        OpenSSL::Digest.new("sha1").name.should == "SHA1"
      end
    end

    it "returns a SHA256 object" do
      NATFIXME 'Implement OpenSSL::Digest#name', exception: NoMethodError, message: "undefined method `name'" do
        OpenSSL::Digest.new("sha256").name.should == "SHA256"
      end
    end

    it "returns a SHA384 object" do
      NATFIXME 'Implement OpenSSL::Digest#name', exception: NoMethodError, message: "undefined method `name'" do
        OpenSSL::Digest.new("sha384").name.should == "SHA384"
      end
    end

    it "returns a SHA512 object" do
      NATFIXME 'Implement OpenSSL::Digest#name', exception: NoMethodError, message: "undefined method `name'" do
        OpenSSL::Digest.new("sha512").name.should ==  "SHA512"
      end
    end

    it "throws an error when called with an unknown digest" do
      -> { OpenSSL::Digest.new("wd40") }.should raise_error(RuntimeError, /Unsupported digest algorithm \(wd40\)/)
    end

    it "cannot be called with a symbol" do
      NATFIXME 'Argument type check in constructor', exception: SpecFailedException do
        -> { OpenSSL::Digest.new(:SHA1) }.should raise_error(TypeError, /wrong argument type Symbol/)
      end
    end

    it "doest not call #to_str on the argument" do
      name = mock("digest name")
      name.should_not_receive(:to_str)
      NATFIXME 'Argument type check in constructor', exception: SpecFailedException do
        -> { OpenSSL::Digest.new(name) }.should raise_error(TypeError, /wrong argument type/)
      end
    end
  end

  describe "can be called with a digest object" do
    it "returns a SHA1 object" do
      NATFIXME 'Allow OpenSSL::Digest instance in constructor', exception: RuntimeError, message: 'Unsupported digest algorithm' do
        OpenSSL::Digest.new(OpenSSL::Digest::SHA1.new).name.should == "SHA1"
      end
    end

    it "returns a SHA256 object" do
      NATFIXME 'Allow OpenSSL::Digest instance in constructor', exception: RuntimeError, message: 'Unsupported digest algorithm' do
        OpenSSL::Digest.new(OpenSSL::Digest::SHA256.new).name.should == "SHA256"
      end
    end

    it "returns a SHA384 object" do
      NATFIXME 'Allow OpenSSL::Digest instance in constructor', exception: RuntimeError, message: 'Unsupported digest algorithm' do
        OpenSSL::Digest.new(OpenSSL::Digest::SHA384.new).name.should == "SHA384"
      end
    end

    it "returns a SHA512 object" do
      NATFIXME 'Allow OpenSSL::Digest instance in constructor', exception: RuntimeError, message: 'Unsupported digest algorithm' do
        OpenSSL::Digest.new(OpenSSL::Digest::SHA512.new).name.should ==  "SHA512"
      end
    end

    it "cannot be called with a digest class" do
      NATFIXME 'Argument type check in constructor', exception: SpecFailedException do
        -> { OpenSSL::Digest.new(OpenSSL::Digest::SHA1) }.should raise_error(TypeError, /wrong argument type Class/)
      end
    end

    it "ignores the state of the name object" do
      NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
        sha1 = OpenSSL::Digest.new('sha1', SHA1Constants::Contents)
        OpenSSL::Digest.new(sha1).digest.should == SHA1Constants::BlankDigest
      end
    end
  end

  describe "ititialization with an empty string" do
    it "returns a SHA1 digest" do
      NATFIXME 'Fix empty string of OpenSSL::Digest#digest', exception: ArgumentError, message: 'wrong number of arguments (given 0, expected 1)' do
        OpenSSL::Digest.new("sha1").digest.should == SHA1Constants::BlankDigest
      end
    end

    it "returns a SHA256 digest" do
      NATFIXME 'Fix empty string of OpenSSL::Digest#digest', exception: ArgumentError, message: 'wrong number of arguments (given 0, expected 1)' do
        OpenSSL::Digest.new("sha256").digest.should == SHA256Constants::BlankDigest
      end
    end

    it "returns a SHA384 digest" do
      NATFIXME 'Fix empty string of OpenSSL::Digest#digest', exception: ArgumentError, message: 'wrong number of arguments (given 0, expected 1)' do
        OpenSSL::Digest.new("sha384").digest.should == SHA384Constants::BlankDigest
      end
    end

    it "returns a SHA512 digest" do
      NATFIXME 'Fix empty string of OpenSSL::Digest#digest', exception: ArgumentError, message: 'wrong number of arguments (given 0, expected 1)' do
        OpenSSL::Digest.new("sha512").digest.should == SHA512Constants::BlankDigest
      end
    end
  end

  describe "can be called with a digest name and data" do
    it "returns a SHA1 digest" do
      NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
        OpenSSL::Digest.new("sha1", SHA1Constants::Contents).digest.should == SHA1Constants::Digest
      end
    end

    it "returns a SHA256 digest" do
      NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
        OpenSSL::Digest.new("sha256", SHA256Constants::Contents).digest.should == SHA256Constants::Digest
      end
    end

    it "returns a SHA384 digest" do
      NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
        OpenSSL::Digest.new("sha384", SHA384Constants::Contents).digest.should == SHA384Constants::Digest
      end
    end

    it "returns a SHA512 digest" do
      NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
        OpenSSL::Digest.new("sha512", SHA512Constants::Contents).digest.should == SHA512Constants::Digest
      end
    end
  end

  context "can be called on subclasses" do
    describe "can be called without data on subclasses" do
      it "returns a SHA1 digest" do
        NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 0, expected 1)' do
          OpenSSL::Digest::SHA1.new.digest.should == SHA1Constants::BlankDigest
        end
      end

      it "returns a SHA256 digest" do
        NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 0, expected 1)' do
          OpenSSL::Digest::SHA256.new.digest.should == SHA256Constants::BlankDigest
        end
      end

      it "returns a SHA384 digest" do
        NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 0, expected 1)' do
          OpenSSL::Digest::SHA384.new.digest.should == SHA384Constants::BlankDigest
        end
      end

      it "returns a SHA512 digest" do
        NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 0, expected 1)' do
          OpenSSL::Digest::SHA512.new.digest.should == SHA512Constants::BlankDigest
        end
      end
    end

    describe "can be called with data on subclasses" do
      it "returns a SHA1 digest" do
        NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 1, expected 0)' do
          OpenSSL::Digest::SHA1.new(SHA1Constants::Contents).digest.should == SHA1Constants::Digest
        end
      end

      it "returns a SHA256 digest" do
        NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 1, expected 0)' do
          OpenSSL::Digest::SHA256.new(SHA256Constants::Contents).digest.should == SHA256Constants::Digest
        end
      end

      it "returns a SHA384 digest" do
        NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 1, expected 0)' do
          OpenSSL::Digest::SHA384.new(SHA384Constants::Contents).digest.should == SHA384Constants::Digest
        end
      end

      it "returns a SHA512 digest" do
        NATFIXME 'Enable constructors', exception: ArgumentError, message: 'wrong number of arguments (given 1, expected 0)' do
          OpenSSL::Digest::SHA512.new(SHA512Constants::Contents).digest.should == SHA512Constants::Digest
        end
      end
    end
  end
end
