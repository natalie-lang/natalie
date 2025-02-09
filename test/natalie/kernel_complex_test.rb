require_relative '../spec_helper'

describe 'Kernel.Complex' do
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
