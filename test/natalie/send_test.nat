require_relative '../spec_helper'

class Foo
  def foo
    'foo'
  end
end

describe 'Kernel#send' do
  it 'works' do
    1.send(:+, 2).should == 3
    Foo.send(:new).send(:foo).should == 'foo'
  end
end
