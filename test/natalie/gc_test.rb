# skip-ruby

require_relative '../spec_helper'
require 'natalie/inline'
require_relative '../support/gc_helpers.cpp'

__function__ 'GC_create_probe', [], 'void'

describe 'GC' do
  def create_object
    $gc_ran = false
    # The native probe sets $gc_ran when it becomes unreachable and is collected.
    __call__('GC_create_probe')
  end

  describe '.start' do
    it 'collects unreachable objects' do
      create_object
      $gc_ran.should == false
      GC.start
      $gc_ran.should == true
    end
  end

  describe '.enable and .disable' do
    it 'control automatic garbage collection' do
      create_object
      $gc_ran.should == false

      GC.disable
      100_000.times { Object.new }
      $gc_ran.should == false

      GC.enable
      10_000.times { Object.new }
      $gc_ran.should == true
    end
  end
end
