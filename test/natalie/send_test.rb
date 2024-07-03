require_relative '../spec_helper'

class Foo
  attr_accessor :arr
  private :arr=

  def foo
    'foo'
  end

  def self_splat_assign(val)
    *self.arr = val
  end
end

describe 'Kernel#send' do
  it 'works' do
    1.send(:+, 2).should == 3
    Foo.send(:new).send(:foo).should == 'foo'
  end
end

describe 'BasicObject#__send__' do
  it 'works' do
    1.__send__(:+, 2).should == 3
    Foo.__send__(:new).__send__(:foo).should == 'foo'
  end
end

describe 'calling send from C++ code to an undefined method' do
  # https://github.com/natalie-lang/natalie/pull/897
  it 'raises a NoMethodError' do
    obj = Object.new
    obj.singleton_class.undef_method(:==)
    -> { 1 == obj }.should raise_error(NoMethodError)
  end
end

describe 'calling multiwrite splat node' do
  it 'works' do
    foo = Foo.new
    foo.self_splat_assign([1, 2])
    foo.arr.should == [1, 2]
  end
end
