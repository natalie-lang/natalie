require_relative '../spec_helper'

# all of these are in the support directory
require 'require_sub1'
load 'require_sub2.rb'
require_relative '../support/require_sub3'
require 'require_sub4.foo'

# this is here to test that order of requires goes top-to-bottom
class Foo1 < Bar1
end
require 'require_sub1'

describe 'require' do
  it 'works' do
    foo1.should == 'foo1'
    Bar1.new.bar1.should == 'bar1'
    foo2.should == 'foo2'
    Bar2.new.bar2.should == 'bar2'
    foo3.should == 'foo3'
    Bar3.new.bar3.should == 'bar3'
  end

  it 'raises an error when the path does not exist' do
    ruby = RUBY_ENGINE == 'natalie' ? 'bin/natalie' : 'ruby'
    `#{ruby} -e "require 'something_non_existent'" 2>&1`.should =~ /cannot load such file.*something_non_existent/
  end

  it 'raises an error when the path is a directory' do
    ruby = RUBY_ENGINE == 'natalie' ? 'bin/natalie' : 'ruby'
    File.directory?('./test').should be_true
    `#{ruby} -e "require './test'" 2>&1`.should =~ /cannot load such file.*test/
  end

  it 'returns true when require loads a file and false when it\'s already loaded' do
    ruby = RUBY_ENGINE == 'natalie' ? 'bin/natalie' : 'ruby'

    `#{ruby} -e "p require 'tempfile'"`.should =~ /true/
    `#{ruby} -e "require 'tempfile'; p require 'tempfile'"`.should =~ /false/
  end
end
