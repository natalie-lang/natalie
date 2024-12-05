require_relative '../spec_helper'

require 'securerandom'

describe 'Random' do
  it 'should extend Random::Formatter' do
    Random.should be_kind_of(Random::Formatter)
  end

  describe 'Random.random_number' do
    it "generates a random float number between 0.0 and 1.0 if argument is negative" do
      num = Random.random_number(-10)
      num.should be_kind_of(Float)
      0.0.should <= num
      num.should < 1.0
    end

    it "generates a random float number between 0.0 and 1.0 if argument is negative float" do
      num = Random.random_number(-11.1)
      num.should be_kind_of(Float)
      0.0.should <= num
      num.should < 1.0
    end

    it "generates a random float number between 0.0 and 1.0 if argument is nil" do
      num = Random.random_number(nil)
      num.should be_kind_of(Float)
      0.0.should <= num
      num.should < 1.0
    end

    it "raises ArgumentError if the argument is non-numeric" do
      obj = Object.new
      -> {
        Random.random_number(obj)
      }.should raise_error(ArgumentError, "invalid argument - #{obj}")
    end

    it "raises Random if the argument is imaginary" do
      -> {
        Random.random_number(1 + 2i)
      }.should raise_error(RangeError)
    end

    it "raises RangeError if the argument is complex" do
      -> {
        Random.random_number(Complex(1, 2))
      }.should raise_error(RangeError)
    end
  end

  describe 'Random.rand' do
    it "raises ArgumentError if argument is negative" do
      -> {
        Random.rand(-10)
      }.should raise_error(ArgumentError, 'invalid argument - -10')
    end

    it "raises ArgumentError if argument is negative float" do
      -> {
        Random.rand(-11.1)
      }.should raise_error(ArgumentError, 'invalid argument - -11.1')
    end

    it "raises ArgumentError if the argument is nil" do
      -> {
        Random.rand(nil)
      }.should raise_error(ArgumentError, 'invalid argument - ')
    end

    it "raises TypeError if the argument is non-numeric" do
      -> {
        Random.rand(Object.new)
      }.should raise_error(TypeError, 'no implicit conversion of Object into Integer')
    end

    it "raises RangeError if the argument is imaginary" do
      -> {
        Random.rand(1 + 2i)
      }.should raise_error(RangeError)
    end

    it "raises RangeError if the argument is complex" do
      -> {
        Random.rand(Complex(1, 2))
      }.should raise_error(RangeError)
    end
  end
end

describe 'SecureRandom' do
  describe 'SecureRandom.random_number' do
    it "generates a random float number between 0.0 and 1.0 if argument is nil" do
      num = SecureRandom.random_number(nil)
      num.should be_kind_of(Float)
      0.0.should <= num
      num.should < 1.0
    end

    it "raises RangeError if the argument is imaginary" do
      -> {
        SecureRandom.random_number(1 + 2i)
      }.should raise_error(RangeError)
    end

    it "raises RangeError if the argument is complex" do
      -> {
        SecureRandom.random_number(Complex(1, 2))
      }.should raise_error(RangeError)
    end
  end
end
