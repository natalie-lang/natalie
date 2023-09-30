require_relative '../../spec/spec_helper'
require 'openssl'

describe "OpenSSL::KDF.pbkdf2_hmac" do
  it "creates the same value with the same input" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = 20_000
    length = 16
    hash = "sha1"
    key = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    key.should == "!\x99+\xF0^\xD0\x8BM\x158\xC4\xAC\x9C\xF1\xF0\xE0".b
  end

  it "supports nullbytes embedded in the password" do
    pass = "sec\x00ret".b
    salt = "\x00".b * 16
    iterations = 20_000
    length = 16
    hash = "sha1"
    key = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    key.should == "\xB9\x7F\xB0\xC2\th\xC8<\x86\xF3\x94Ij7\xEF\xF1".b
  end

  it "coerces the password into a String using #to_str" do
    pass = mock("pass")
    pass.should_receive(:to_str).and_return("secret")
    salt = "\x00".b * 16
    iterations = 20_000
    length = 16
    hash = "sha1"
    key = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    key.should == "!\x99+\xF0^\xD0\x8BM\x158\xC4\xAC\x9C\xF1\xF0\xE0".b
  end

  it "coerces the salt into a String using #to_str" do
    pass = "secret"
    salt = mock("salt")
    salt.should_receive(:to_str).and_return("\x00".b * 16)
    iterations = 20_000
    length = 16
    hash = "sha1"
    key = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    key.should == "!\x99+\xF0^\xD0\x8BM\x158\xC4\xAC\x9C\xF1\xF0\xE0".b
  end

  it "coerces the iterations into an Integer using #to_int" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = mock("iterations")
    iterations.should_receive(:to_int).and_return(20_000)
    length = 16
    hash = "sha1"
    key = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    key.should == "!\x99+\xF0^\xD0\x8BM\x158\xC4\xAC\x9C\xF1\xF0\xE0".b
  end

  it "coerces the length into an Integer using #to_int" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = 20_000
    length = mock("length")
    length.should_receive(:to_int).and_return(16)
    hash = "sha1"
    key = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    key.should == "!\x99+\xF0^\xD0\x8BM\x158\xC4\xAC\x9C\xF1\xF0\xE0".b
  end

  it "accepts a OpenSSL::Digest object as hash" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = 20_000
    length = 16
    hash = OpenSSL::Digest.new("sha1")
    key = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    key.should == "!\x99+\xF0^\xD0\x8BM\x158\xC4\xAC\x9C\xF1\xF0\xE0".b
  end

  it "accepts an empty password" do
    pass = ""
    salt = "\x00".b * 16
    iterations = 20_000
    length = 16
    hash = "sha1"
    key = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    key.should == "k\x9F-\xB1\xF7\x9A\v\xA1(C\xF9\x85!P\xEF\x8C".b
  end

  it "accepts an empty salt" do
    pass = "secret"
    salt = ""
    iterations = 20_000
    length = 16
    hash = "sha1"
    key = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    key.should == "\xD5f\xE5\xEA\xF91\x1D\xD3evD\xED\xDB\xE80\x80".b
  end

  it "accepts an empty length" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = 20_000
    length = 0
    hash = "sha1"
    key = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    key.should.empty?
  end

  it "accepts an arbitrary length" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = 20_000
    length = 19
    hash = "sha1"
    key = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    key.should == "!\x99+\xF0^\xD0\x8BM\x158\xC4\xAC\x9C\xF1\xF0\xE0\xCF\xBB\x7F".b
  end

  it "accepts any hash function known to OpenSSL" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = 20_000
    length = 16
    hash = "sha512"
    key = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    key.should == "N\x12}D\xCE\x99\xDBC\x8E\xEC\xAAr\xEA1\xDF\xFF".b
  end

  it "raises a TypeError when password is not a String and does not respond to #to_str" do
    pass = Object.new
    salt = "\x00".b * 16
    iterations = 20_000
    length = 16
    hash = "sha1"
    -> {
      OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    }.should raise_error(TypeError, "no implicit conversion of Object into String")
  end

  it "raises a TypeError when salt is not a String and does not respond to #to_str" do
    pass = "secret"
    salt = Object.new
    iterations = 20_000
    length = 16
    hash = "sha1"
    -> {
      OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    }.should raise_error(TypeError, "no implicit conversion of Object into String")
  end

  it "raises a TypeError when iterations is not an Integer and does not respond to #to_int" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = Object.new
    length = 16
    hash = "sha1"
    -> {
      OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    }.should raise_error(TypeError, "no implicit conversion of Object into Integer")
  end

  it "raises a TypeError when length is not an Integer and does not respond to #to_int" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = 20_000
    length = Object.new
    hash = "sha1"
    -> {
      OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    }.should raise_error(TypeError, "no implicit conversion of Object into Integer")
  end

  it "raises a TypeError when hash is neither a String nor an OpenSSL::Digest" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = 20_000
    length = 16
    hash = Object.new
    -> {
      OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    }.should raise_error(TypeError, "wrong argument type Object (expected OpenSSL/Digest)")
  end

  it "raises a TypeError when hash is neither a String nor an OpenSSL::Digest, it does not try to call #to_str" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = 20_000
    length = 16
    hash = mock("hash")
    hash.should_not_receive(:to_str)
    -> {
      OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    }.should raise_error(TypeError, "wrong argument type MockObject (expected OpenSSL/Digest)")
  end

  it "raises a RuntimeError for unknown digest algorithms" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = 20_000
    length = 16
    hash = "wd40"
    -> {
      OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
    }.should raise_error(RuntimeError, /Unsupported digest algorithm \(wd40\)/)
  end

  it "treats salt as a required keyword" do
    pass = "secret"
    iterations = 20_000
    length = 16
    hash = "sha1"
    -> {
      OpenSSL::KDF.pbkdf2_hmac(pass, iterations: iterations, length: length, hash: hash)
    }.should raise_error(ArgumentError, 'missing keyword: :salt')
  end

  it "treats iterations as a required keyword" do
    pass = "secret"
    salt = "\x00".b * 16
    length = 16
    hash = "sha1"
    -> {
      OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, length: length, hash: hash)
    }.should raise_error(ArgumentError, 'missing keyword: :iterations')
  end

  it "treats length as a required keyword" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = 20_000
    hash = "sha1"
    -> {
      OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, hash: hash)
    }.should raise_error(ArgumentError, 'missing keyword: :length')
  end

  it "treats hash as a required keyword" do
    pass = "secret"
    salt = "\x00".b * 16
    iterations = 20_000
    length = 16
    -> {
      OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length)
    }.should raise_error(ArgumentError, 'missing keyword: :hash')
  end

  it "treats all keywords as required" do
    pass = "secret"
    -> {
      OpenSSL::KDF.pbkdf2_hmac(pass)
    }.should raise_error(ArgumentError, 'missing keywords: :salt, :iterations, :length, :hash')
  end


  it "has behaviour for 0 or less iterations that depends on the version of OpenSSL" do
    pass = "secret"
    salt = "\x00".b * 16
    length = 16
    hash = "sha1"
    if OpenSSL::VERSION.split('.').first.to_i < 3
      # "Any iter less than 1 is treated as a single iteration."
      iterations = 0
      key0 = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
      iterations = -1
      key_negative = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
      iterations = 1
      key1 = OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
      key0.should == key1
      key_negative.should == key1
    else
      # This raises an error (even though the OpenSSL docs say otherwise)
      iterations = 0
      -> {
        OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
      }.should raise_error(OpenSSL::KDF::KDFError, "PKCS5_PBKDF2_HMAC: invalid iteration count")

      iterations = -1
      -> {
        OpenSSL::KDF.pbkdf2_hmac(pass, salt: salt, iterations: iterations, length: length, hash: hash)
      }.should raise_error(OpenSSL::KDF::KDFError, /PKCS5_PBKDF2_HMAC/)
    end
  end
end
