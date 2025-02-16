require_relative '../spec_helper'

describe 'Kernel.Complex' do
  it 'only accepts true or false as exception argument' do
    -> { Complex('1', exception: nil) }.should raise_error(ArgumentError, 'expected true or false as exception: nil')
    obj = Object.new
    -> { Complex('1', exception: obj) }.should raise_error(ArgumentError, "expected true or false as exception: #{obj}")
  end

  it 'does not accept anything nun-numeric after the real integer part' do
    -> { Complex('1a') }.should raise_error(ArgumentError, 'invalid value for convert(): "1a"')
    -> { Complex('+1a') }.should raise_error(ArgumentError, 'invalid value for convert(): "+1a"')
    -> { Complex('-1a') }.should raise_error(ArgumentError, 'invalid value for convert(): "-1a"')
  end

  it 'does not accept anything nun-numeric after the real float part' do
    -> { Complex('1.0a') }.should raise_error(ArgumentError, 'invalid value for convert(): "1.0a"')
    -> { Complex('+1.0a') }.should raise_error(ArgumentError, 'invalid value for convert(): "+1.0a"')
    -> { Complex('-1.0a') }.should raise_error(ArgumentError, 'invalid value for convert(): "-1.0a"')
    -> { Complex('1.a') }.should raise_error(ArgumentError, 'invalid value for convert(): "1.a"')
    -> { Complex('+1.a') }.should raise_error(ArgumentError, 'invalid value for convert(): "+1.a"')
    -> { Complex('-1.a') }.should raise_error(ArgumentError, 'invalid value for convert(): "-1.a"')
  end

  it 'does not accept two dots in the real part' do
    -> { Complex('1.0.') }.should raise_error(ArgumentError, 'invalid value for convert(): "1.0."')
    -> { Complex('1..0') }.should raise_error(ArgumentError, 'invalid value for convert(): "1..0"')
  end

  it "accepts an 'e' only once per numeric part" do
    -> { Complex('1e2e3') }.should raise_error(ArgumentError, 'invalid value for convert(): "1e2e3"')
    -> { Complex('1e2e3+1i') }.should raise_error(ArgumentError, 'invalid value for convert(): "1e2e3+1i"')
    -> { Complex('1+1e2e3i') }.should raise_error(ArgumentError, 'invalid value for convert(): "1+1e2e3i"')
  end

  it 'ignores whitespace' do
    Complex("\t1").should == 1
    Complex("\n1").should == 1
    Complex("\v1").should == 1
    Complex("\f1").should == 1
    Complex("\r1").should == 1
  end

  it 'stops on non-printable ascii chars' do
    ->{ Complex("\x001".b) }.should raise_error(ArgumentError, 'string contains null byte')
    (Array(0x01..0x08) + Array(0x0E..0x1F) + [0x7F]).each do |c|
      -> { Complex("#{c.chr(Encoding::ASCII_8BIT)}1") }.should raise_error(ArgumentError, /invalid value for convert\(\):/)
    end
  end
end
