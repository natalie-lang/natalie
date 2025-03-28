require_relative '../spec_helper'

class Foo
  def foo
    'foo'
  end

  def double(x)
    x * 2
  end
end

describe 'class' do
  describe 'instance methods' do
    it 'calls the method and returns the result' do
      foo = Foo.new
      foo.foo.should == 'foo'
      Foo.new.foo.should == 'foo'
      Foo.new.double(4).should == 8
    end
  end
end

# reopen class
class Foo
  def foo2
    'foo2'
  end
end

describe 'class' do
  describe 'reopen class' do
    it 'has both old and new methods' do
      Foo.new.foo2.should == 'foo2'
      Foo.new.foo.should == 'foo'
    end

    it 'should check if the reopened constant really is a class' do
      -> do
        class RUBY_VERSION
        end
      end.should raise_error(TypeError, /RUBY_VERSION is not a class/)

      -> do
        class Comparable
        end
      end.should raise_error(TypeError, /Comparable is not a class/)
    end
  end
end

class Bar < Foo
  def foo
    'bar'
  end
end

WAT = 1

describe 'class' do
  describe 'inheritance' do
    it 'has its own methods and those of its superclass' do
      Bar.new.foo.should == 'bar'
      Bar.new.double(3).should == 6
    end

    it 'raises an error if the superclass is not a class' do
      -> do
        class WatClass < WAT
        end
      end.should raise_error(TypeError, 'superclass must be an instance of Class (given an instance of Integer)')
      -> { Class.new(WAT) }.should raise_error(
                   TypeError,
                   'superclass must be an instance of Class (given an instance of Integer)',
                 )
    end

    it 'raises an error if the superclass is Class' do
      -> do
        class WatClass < Class
        end
      end.should raise_error(TypeError, "can't make subclass of Class")
      -> { Class.new(Class) }.should raise_error(TypeError, "can't make subclass of Class")
    end

    it 'raises an error if the superclass is a singleton class' do
      -> do
        class WatClass < Object.singleton_class
        end
      end.should raise_error(TypeError, "can't make subclass of singleton class")
      -> { Class.new(Object.singleton_class) }.should raise_error(TypeError, "can't make subclass of singleton class")
    end
  end
end

class Baz < Foo
  def foo
    super + ' baz' if true
  end
end

describe 'class' do
  describe 'super' do
    it 'calls the same method on the superclass' do
      Baz.new.foo.should == 'foo baz'
    end
  end
end

module M0
end

module M1
  def m1
    'm1'
  end
end

module M2
  def m2
    'm2'
  end
end

module M3
end

# reopen module
module M1
  def m1b
    'm1b'
  end
end

class ExtendTest
  extend M1, M2
end

class IncludeTest
  prepend M0
  include M1, M2
  include M3
end

class IncludeTestOverride
  include M1

  def m1
    'my m1'
  end
end

class PrependTest
  prepend M1, M2
end

class PrependTestOverride
  prepend M1

  def m1
    'my m1'
  end
end

describe 'class' do
  describe 'extend' do
    it 'includes methods from a module into the singleton class' do
      ExtendTest.ancestors.should == [ExtendTest, Object, Kernel, BasicObject]
      ExtendTest.m1.should == 'm1'
      ExtendTest.m1b.should == 'm1b'
    end
  end

  describe 'include' do
    it 'includes methods from a module' do
      IncludeTest.ancestors.should == [M0, IncludeTest, M3, M1, M2, Object, Kernel, BasicObject]
      IncludeTest.new.m1.should == 'm1'
      IncludeTest.new.m1b.should == 'm1b'
      IncludeTestOverride.new.m1.should == 'my m1'
    end
  end

  describe 'prepend' do
    it 'prepends modules in the lookup chain' do
      PrependTest.ancestors.should == [M1, M2, PrependTest, Object, Kernel, BasicObject]
      PrependTest.new.m1.should == 'm1'
      PrependTest.new.m1b.should == 'm1b'
      PrependTestOverride.new.m1.should == 'm1'
    end
  end
end

class IvarTest
  def add_w(w)
    @w ||= []
    @w << w
  end

  def set_x(x)
    @x = x
  end

  def get_x
    @x
  end

  def set_y(y)
    instance_variable_set(:@y, y)
  end

  def get_y
    instance_variable_get(:@y)
  end

  def set_z(z)
    @z = z
  end

  def block_test
    result = []
    [1].each { |i| result << @z }
    result
  end

  attr_reader :w
  attr_reader :z
  attr_reader :z2
  attr_writer :z
  attr_writer :z2
  attr_accessor :zz
  attr_accessor :zz2
end

describe 'class' do
  describe 'instance variables' do
    it 'can be set and returned' do
      i = IvarTest.new
      i.w.should == nil
      i.add_w 1
      i.w.should == [1]
      i.add_w 2
      i.w.should == [1, 2]
      i.get_x.should == nil
      i.set_x(10)
      i.get_x.should == 10
      i.set_x(20)
      i.get_x.should == 20
      i.set_y(10)
      i.get_y.should == 10
      i.set_y(20)
      i.get_y.should == 20
      i.z.should == nil
      i.z = 10
      i.z.should == 10
      i.z = 20
      i.z2 = 200
      i.z.should == 20
      i.z2.should == 200
      i.zz = 30
      i.zz2 = 300
      i.zz.should == 30
      i.zz2.should == 300
      i.block_test.should == [20]
    end
  end
end

class InitMethodTest
  def initialize
    @x = 1
  end

  def x
    @x
  end
end

class InitMethodTest2
  def initialize(x)
    @x = x
  end

  def x
    @x
  end
end

class InitMethodTest3 < InitMethodTest2
  def x
    @x
  end
end

module InitMethodModule
  def initialize(x)
    @x = x
  end
end

class InitMethodTest4
  include InitMethodModule

  def x
    @x
  end
end

describe 'class' do
  describe 'initialize method' do
    it 'is called when the object is created' do
      InitMethodTest.new.x.should == 1
    end

    it 'is passed arguments' do
      InitMethodTest2.new(2).x.should == 2
    end

    it 'is called on superclass if not present' do
      InitMethodTest3.new(3).x.should == 3
    end

    it 'is called on a module' do
      InitMethodTest4.new(4).x.should == 4
    end
  end
end

describe 'built-in class/object hierarchy' do
  it 'matches ruby' do
    Integer.class.inspect.should == 'Class'
    Class.class.inspect.should == 'Class'
    Class.superclass.inspect.should == 'Module'
    Module.class.inspect.should == 'Class'
    Module.superclass.inspect.should == 'Object'
    Integer.superclass.inspect.should == 'Numeric'
    Numeric.superclass.inspect.should == 'Object'
    Object.superclass.inspect.should == 'BasicObject'
    BasicObject.class.inspect.should == 'Class'
    BasicObject.superclass.inspect.should == 'nil'
  end
end

class SingletonAccess
  class << self
    def foo
      'foo'
    end
  end
end

describe 'class << self' do
  it 'changes self to the singleton class' do
    SingletonAccess.foo.should == 'foo'
    -> { SingletonAccess.new.foo }.should raise_error(NoMethodError)
  end
end

class AliasTest
  def foo
    'foo'
  end

  alias bar foo
end

describe 'alias' do
  AliasTest.new.bar.should == 'foo'
  AliasTest.alias_method :baz, :foo
  AliasTest.new.baz.should == 'foo'
end

BrokenClass =
  Class.new do
    1
    break
  end

describe 'break inside new block' do
  it 'causes the returned value to be nil' do
    BrokenClass.should be_nil
  end
end

class Foo2
  class SameName
  end
end

class Foo3 < Foo2
  module M1
    def m1
      'my-m1'
    end
  end

  include M1

  class SameName
    include M1
  end
end

describe 'including a module from the parent namespace' do
  it 'includes the correct module' do
    Foo3.new.m1.should == 'my-m1'
    Foo3::SameName.new.m1.should == 'my-m1'
  end

  it 'does not reopen a class from the namespace of a superclass' do
    Foo2::SameName.should_not equal(Foo3::SameName)
  end
end
