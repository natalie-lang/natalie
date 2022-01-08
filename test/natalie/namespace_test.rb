require_relative '../spec_helper'

NUM = 1

class Foo
  NUM = 2

  def self.num
    NUM
  end

  def num
    NUM
  end
end

class Bar < Foo
  NUM = 3

  def self.num
    NUM
  end

  def num
    NUM
  end
end

class Baz < Bar
  def self.num
    NUM
  end

  def num
    NUM
  end
end

module Mod
  class C1
    class C1a
    end
  end

  class C2
    Copy = C1
    def c1
      C1
    end
  end
end

module PrecedenceTest
  VAL = 1

  class A
    VAL = 2

    def self.val
      VAL
    end
  end

  class B < A
    def self.val
      VAL
    end
  end

  class C
    def self.val
      VAL
    end
  end
end

class Colon3
  class Foo
    NUM = 100
  end

  def self.my_foo
    Foo
  end

  def my_foo
    Foo
  end

  def self.top_foo
    ::Foo
  end

  def top_foo
    ::Foo
  end

  def self.my_foo_num
    Foo::NUM
  end

  def my_foo_num
    Foo::NUM
  end

  def self.top_foo_num
    ::Foo::NUM
  end

  def top_foo_num
    ::Foo::NUM
  end
end

module NestedTest
end
NestedTest::NESTED_CONSTANT = nil
class NestedTest::NestedClass
end
module NestedTest::NestedModule
end

module GlobalTest
  ::NESTED_GLOBAL_CONSTANT = nil
  class ::NestedGlobalClass
  end
  module ::NestedGlobalModule
  end
end

GLOBAL_CONSTANT = nil

describe 'namespace' do
  it 'resolves top-level constants' do
    NUM.should == 1
  end

  it 'resolves constants on a class' do
    Foo::NUM.should == 2
    Foo.num.should == 2
    Foo.num.should == 2
    Foo.new.num.should == 2
    Bar::NUM.should == 3
    Bar.num.should == 3
    Bar.num.should == 3
    Bar.new.num.should == 3
    Baz::NUM.should == 3
    Baz.num.should == 3
    Baz.num.should == 3
    Baz.new.num.should == 3
  end

  it 'walks up the namespaces to find constants' do
    Mod::C2.new.c1.should == Mod::C1
  end

  it 'includes the namespace when inspecting' do
    Mod::C1.inspect.should == 'Mod::C1'
    Mod::C1::C1a.inspect.should == 'Mod::C1::C1a'
    Mod::C2.inspect.should == 'Mod::C2'
    Mod::C2::Copy.inspect.should == 'Mod::C1'
  end

  it 'looks for constants with proper precedence' do
    PrecedenceTest::A::VAL.should == 2
    PrecedenceTest::A.val.should == 2
    PrecedenceTest::B::VAL.should == 2
    PrecedenceTest::B.val.should == 1
    PrecedenceTest::C.val.should == 1
  end

  it 'looks in the current namespace unless prefixed with ::' do
    Colon3.my_foo.should == Colon3::Foo
    Colon3.new.my_foo.should == Colon3::Foo
    Colon3.my_foo_num.should == 100
    Colon3.new.my_foo_num.should == 100
    Colon3.top_foo.should == Foo
    Colon3.new.top_foo.should == Foo
    Colon3.top_foo_num.should == 2
    Colon3.new.top_foo_num.should == 2
  end

  it 'allows defining nested constants from outside the namespace' do
    -> { NestedTest::NESTED_CONSTANT }.should_not raise_error
    -> { NestedTest::NestedClass }.should_not raise_error
    -> { NestedTest::NestedModule }.should_not raise_error
  end

  it 'allows defining global constants from inside a namespace' do
    -> { NESTED_GLOBAL_CONSTANT }.should_not raise_error
    -> { NestedGlobalClass }.should_not raise_error
    -> { NestedGlobalModule }.should_not raise_error
    -> { GlobalTest::NESTED_GLOBAL_CONSTANT }.should raise_error(
                                                       NameError,
                                                       /uninitialized constant GlobalTest::NESTED_GLOBAL_CONSTANT/,
                                                     )
    -> { GlobalTest::NestedGlobalClass }.should raise_error(
                                                  NameError,
                                                  /uninitialized constant GlobalTest::NestedGlobalClass/,
                                                )
    -> { GlobalTest::NestedGlobalModule }.should raise_error(
                                                   NameError,
                                                   /uninitialized constant GlobalTest::NestedGlobalModule/,
                                                 )
  end

  it 'stores global constants in Object class' do
    -> { Object::GLOBAL_CONSTANT }.should_not raise_error
    -> { GlobalTest::GLOBAL_CONSTANT }.should raise_error(
                                                NameError,
                                                /uninitialized constant GlobalTest::GLOBAL_CONSTANT/,
                                              )
  end

  it 'raises a NameError when the given namespace is not initialized' do
    -> {
      class NonexistentNamespace::Foo
      end
    }.should raise_error(NameError, /uninitialized constant NonexistentNamespace/)
    -> {
      module NonexistentNamespace::Foo
      end
    }.should raise_error(NameError, /uninitialized constant NonexistentNamespace/)
  end
end
