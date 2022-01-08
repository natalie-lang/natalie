require_relative '../spec_helper'

describe 'to_enum' do
  it 'always invokes block for size when block is provided' do
    a = 0
    b = [].to_enum { a += 1 }
    b.size.should == 1
    b.size.should == 2

    b = [].to_enum.lazy.to_enum { a += 1 }
    b.size.should == 3
    b.size.should == 4
  end
end
