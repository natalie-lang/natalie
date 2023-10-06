require_relative '../spec_helper'

$LOAD_PATH << File.expand_path('../support', __dir__)

module Foo
  autoload :Simple, 'autoload/simple'
  autoload :UpALevel, 'autoload/up_a_level'
  autoload :Nested, 'autoload/nested'
  autoload :Missing, 'autoload/missing'
end

describe 'autoload' do
  it 'loads the file when the constant is referenced' do
    $foo_simple_loaded.should == nil
    Foo::Simple.should be_an_instance_of(Class)
    $foo_simple_loaded.should == true
  end

  it 'searches up the module hierarchy for the constant' do
    $foo_nested_loaded.should == nil
    Foo::Nested.should be_an_instance_of(Class)
    Foo::Nested::A.should be_an_instance_of(Class)
    Foo::Nested::A.superclass.should == Foo::UpALevel
    $foo_nested_loaded.should == true
  end

  it 'raises an error when the file does not define the constant' do
    $missing_loaded.should == nil
    -> { Foo::Missing }.should raise_error(NameError, /uninitialized constant Foo::Missing/)
    $missing_loaded.should == true
  end
end
