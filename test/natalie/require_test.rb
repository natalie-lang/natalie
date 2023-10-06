require_relative '../spec_helper'

# test/support/require
# ├── cpp_file.cpp
# ├── loaded.rb
# ├── relative.rb
# ├── simple
# │   └── relative.rb
# ├── simple_again.rb
# ├── simple.rb
# ├── with_fake_ext.ext
# └── with_fake_ext.ext.rb

$LOAD_PATH << File.expand_path('../support', __dir__)

require 'require/simple'
load 'require/loaded.rb'
require_relative '../support/require/relative'
require 'require/with_fake_ext.ext'
begin
  require 'require/cpp_file'
rescue LoadError
  # expected in MRI since this is a cpp file
end

# this is here to test that order of requires goes top-to-bottom
class Foo1Child < Foo1
end
require 'require/simple' # rubocop:disable Lint/DuplicateRequire

describe 'require' do
  it 'requires a file from the load path' do
    simple.should == 'simple'
  end

  it 'loads a file by full name' do
    loaded.should == 'loaded'
  end

  it 'requires a relative file' do
    relative.should == 'relative'
  end

  it 'requires a relative file from another file' do
    simple_relative.should == 'simple_relative'
    Foo1::Bar1.should be_an_instance_of(Class)
  end

  it 'requires a cpp file' do
    if RUBY_ENGINE == 'natalie'
      cpp_file.should == 'cpp_file'
    end
  end

  it 'raises an error when the path does not exist' do
    lambda do
      require 'something_non_existent'
    end.should raise_error(
      LoadError,
      /cannot load such file.*something_non_existent/
    )
  end

  it 'raises an error when the path is a directory' do
    lambda do
      require_relative '../../test'
    end.should raise_error(
      LoadError,
      /cannot load such file.*test/
    )
  end

  it 'returns true when require loads a file and false when it\'s already loaded' do
    result1 = require 'require/simple_again'
    result1.should == true
    simple_again.should == 'simple_again'
    result2 = require 'require/simple_again'
    result2.should == false
    simple_again.should == 'simple_again'
  end

  it 'works in the middle of a method' do
    require 'socket'
    Socket.should be_an_instance_of(Class)
  end
end
