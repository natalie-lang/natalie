require_relative '../spec_helper'

describe 'Kernel.Complex' do
  it 'does not accept anything nun-numeric after the real integer part' do
    -> { Complex('1a') }.should raise_error(ArgumentError, 'invalid value for convert(): "1a"')
    -> { Complex('+1a') }.should raise_error(ArgumentError, 'invalid value for convert(): "+1a"')
    -> { Complex('-1a') }.should raise_error(ArgumentError, 'invalid value for convert(): "-1a"')
  end
end
