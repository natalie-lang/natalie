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
end
