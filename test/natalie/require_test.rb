require_relative '../spec_helper'

# all of these are in the support directory
require 'require_sub1'
load 'require_sub2.rb'
require_relative '../support/require_sub3'
require 'require_sub4.foo'
begin
  require 'require_sub5'
rescue LoadError
  # expected in MRI since this is a cpp file
end

# this is here to test that order of requires goes top-to-bottom
class Foo1 < Bar1
end
require 'require_sub1' # rubocop:disable Lint/DuplicateRequire

describe 'require' do
  it 'works' do
    foo1.should == 'foo1'
    Bar1.new.bar1.should == 'bar1'
    foo2.should == 'foo2'
    Bar2.new.bar2.should == 'bar2'
    foo3.should == 'foo3'
    Bar3.new.bar3.should == 'bar3'
    foo4.should == 'foo4'
    if RUBY_ENGINE == 'natalie'
      foo5.should == 'foo5'
    end
  end

  it 'raises an error when the path does not exist' do
    ruby = RUBY_ENGINE == 'natalie' ? NAT_BINARY : 'ruby'
    `#{ruby} -e "require 'something_non_existent'" 2>&1`.should =~ /cannot load such file.*something_non_existent/
  end

  it 'raises an error when the path is a directory' do
    ruby = RUBY_ENGINE == 'natalie' ? NAT_BINARY : 'ruby'
    File.directory?('./test').should be_true
    `#{ruby} -e "require './test'" 2>&1`.should =~ /cannot load such file.*test/
  end

  it 'returns true when require loads a file and false when it\'s already loaded' do
    ruby = RUBY_ENGINE == 'natalie' ? NAT_BINARY : 'ruby'

    `#{ruby} -e "p require 'tempfile'"`.should =~ /true/
    `#{ruby} -e "require 'tempfile'; p require 'tempfile'"`.should =~ /false/
  end

  it 'works in the middle of a method' do
    require 'socket'
    Socket.should be_an_instance_of(Class)
  end
end
