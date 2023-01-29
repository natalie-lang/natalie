require_relative '../spec_helper'

class Foo
end

class Bar < Foo
end

describe 'singleton_class' do
  it 'has the correct hierarchy' do
    klass = Bar.singleton_class
    klass.inspect.should == '#<Class:Bar>'
    klass = klass.superclass
    klass.inspect.should == '#<Class:Foo>'
    klass = klass.superclass
    klass.inspect.should == '#<Class:Object>'
    klass = klass.superclass
    klass.inspect.should == '#<Class:BasicObject>'
    klass = klass.superclass
    klass.should == Class
    klass = klass.superclass
    klass.should == Module
    klass = klass.superclass
    klass.should == Object
    klass = klass.superclass
    klass.should == BasicObject
    klass = klass.superclass
    klass.should == nil

    klass = Class.singleton_class
    klass.inspect.should == '#<Class:Class>'
    klass = klass.superclass
    klass.inspect.should == '#<Class:Module>'
    klass = klass.superclass
    klass.inspect.should == '#<Class:Object>'
    klass = klass.superclass
    klass.inspect.should == '#<Class:BasicObject>'
    klass = klass.superclass
    klass.should == Class
  end

  it 'knows it is a singleton class' do
    Bar.singleton_class.singleton_class?.should == true
    Bar.singleton_class?.should == false
  end

  it 'has the correct name if not a module or class' do
    o = Object.new
    klass = o.singleton_class
    klass.inspect.should == "#<Class:#{o.inspect}>"
    klass.singleton_class.inspect.should == "#<Class:#<Class:#{o.inspect}>>"
  end

  it 'schrodingers singleton classes' do
    nil.singleton_class.singleton_class?.should == false
    true.singleton_class.singleton_class?.should == false
    false.singleton_class.singleton_class?.should == false
  end

  # NATFIXME: Remove this and sync upstream spec once https://github.com/ruby/spec/pull/1008 is merged
  it 'is frozen if the object it is created from is frozen' do
    o = Object.new
    o.freeze
    klass = o.singleton_class
    klass.frozen?.should == true
  end

  # NATFIXME: Remove this and sync upstream spec once https://github.com/ruby/spec/pull/1008 is merged
  it 'will be frozen if the object it is created from becomes frozen' do
    o = Object.new
    klass = o.singleton_class
    klass.frozen?.should == false
    o.freeze
    klass.frozen?.should == true
  end

  # NATFIXME: Remove this and sync upstream spec once https://github.com/ruby/spec/pull/1008 is merged
  it 'cannot define a method on a frozen singleton class' do
    o = Object.new
    o.freeze
    klass = o.singleton_class
    -> { o.define_singleton_method(:foo) { 1 } }.should raise_error(FrozenError)
  end
end
