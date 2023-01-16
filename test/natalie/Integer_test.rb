require_relative '../spec_helper'

describe 'Kernel.Integer' do
  it 'Does not accept a any whitespace after a sign mark' do
    -> { Integer("+\x001".b) }.should raise_error(ArgumentError, 'invalid value for Integer(): "+\\x001"')
    -> { Integer("+\t1") }.should raise_error(ArgumentError, 'invalid value for Integer(): "+\\t1"')
    -> { Integer("+\n1") }.should raise_error(ArgumentError, 'invalid value for Integer(): "+\\n1"')
    -> { Integer("+\v1") }.should raise_error(ArgumentError, 'invalid value for Integer(): "+\\v1"')
    -> { Integer("+\f1") }.should raise_error(ArgumentError, 'invalid value for Integer(): "+\\f1"')
    -> { Integer("+\r1") }.should raise_error(ArgumentError, 'invalid value for Integer(): "+\\r1"')
    -> { Integer("+ 1") }.should raise_error(ArgumentError, 'invalid value for Integer(): "+ 1"')
  end
end
