require_relative '../spec_helper'

$LOAD_PATH << File.expand_path('../support', __dir__)

module Foo
  autoload :Simple, 'autoload/simple'
  autoload :UpALevel, 'autoload/up_a_level'
  autoload :Nested, 'autoload/nested'
  autoload :Missing, 'autoload/missing'
  autoload :Query, 'autoload/query'
  autoload :SameFile1, 'autoload/same_file'
  autoload :SameFile2, 'autoload/same_file'
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

  it 'can load more than one constant from the same file' do
    $same_file_loaded.should == nil
    Foo::SameFile2.should be_an_instance_of(Class)
    Foo::SameFile1.should be_an_instance_of(Class)
    $same_file_loaded.should == true
  end

  it 'raises an error when the file does not define the constant' do
    $missing_loaded.should == nil
    -> { Foo::Missing }.should raise_error(NameError, /uninitialized constant Foo::Missing/)
    $missing_loaded.should == true
  end
end

describe 'Module#autoload?' do
  it 'returns the path when the constant name is yet to be autoload' do
    Foo.autoload?(:Query).should == 'autoload/query'
    Foo.autoload?('Query').should == 'autoload/query'
    Foo::Query.should be_an_instance_of(Class)
  end

  it 'returns nil once the constant is loaded' do
    Foo.autoload?(:Query).should == nil
  end

  it 'raises a TypeError for other kinds of arguments' do
    -> { Foo.autoload?(1) }.should raise_error(TypeError, '1 is not a symbol nor a string')
    -> { Foo.autoload?(nil) }.should raise_error(TypeError, 'nil is not a symbol nor a string')
  end
end
