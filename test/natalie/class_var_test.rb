require_relative '../spec_helper'

class CvarTest1
  def self.set_foo(x)
    @@foo = x
  end

  def self.foo
    @@foo
  end
end

class CvarTest2
  def set_foo(x)
    @@foo = x
  end

  def foo
    @@foo
  end
end

class CvarTest3a
  def self.set_foo(x)
    @@foo = x
  end

  def self.foo
    @@foo
  end
end

class CvarTest3b < CvarTest3a
end

class CvarTest4a
  def set_foo(x)
    @@foo = x
  end

  def foo
    @@foo
  end
end

class CvarTest4b < CvarTest4a
end

class CvarTest5
  def self.foo
    @@foo
  end
end

class CvarTest6
  @@foo = 100

  def self.foo
    @@foo
  end
end

class CvarTest7
  def self.add_foo(x)
    @@foo ||= []
    @@foo << x
  end

  def self.set_foo(val)
    @@foo = val
  end

  def self.foo
    @@foo
  end
end

describe 'class variable' do
  it 'can bet set and retrieved on class methods' do
    CvarTest1.set_foo(1)
    CvarTest1.foo.should == 1
  end

  it 'can bet set and retrieved on instance methods' do
    f = CvarTest2.new
    f.set_foo(1)
    f.foo.should == 1
  end

  it 'can bet set and retrieved on class methods of a subclass' do
    CvarTest3a.set_foo(1)
    CvarTest3a.foo.should == 1
    CvarTest3b.foo.should == 1
    CvarTest3b.set_foo(10)
    CvarTest3a.foo.should == 10
    CvarTest3b.foo.should == 10
  end

  it 'can bet set and retrieved on instance methods of a subclass' do
    f = CvarTest4a.new
    f.set_foo(1)
    f.foo.should == 1
    b = CvarTest4b.new
    b.foo.should == 1
    b.set_foo(10)
    f.foo.should == 10
    b.foo.should == 10
  end

  it 'raises an exception when attempting to retreive a class variable not set' do
    -> { CvarTest5.foo }.should raise_error(NameError)
  end

  it 'can bet set in the class body' do
    CvarTest6.foo.should == 100
  end

  it 'can be set with an op_asgn_or' do
    CvarTest7.add_foo 1000
    CvarTest7.foo.should == [1000]
    CvarTest7.add_foo 2000
    CvarTest7.foo.should == [1000, 2000]
    CvarTest7.set_foo nil
    CvarTest7.add_foo 1000
    CvarTest7.foo.should == [1000]
    CvarTest7.set_foo false
    CvarTest7.add_foo 1000
    CvarTest7.foo.should == [1000]
    CvarTest7.set_foo 'x'
    CvarTest7.foo.should == 'x'
  end
end
