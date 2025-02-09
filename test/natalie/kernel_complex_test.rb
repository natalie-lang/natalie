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
end
