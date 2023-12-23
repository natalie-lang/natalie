# skip-ruby

require_relative '../spec_helper'
require 'natalie/inline'

describe 'GC' do
  def create_object
    $gc_ran = false
    ptr = nil
    # Our VoidPObject has a cleanup function that runs whenever
    # the object is collected by the GC, so we can use that to
    # test that it did indeed run.
    __inline__ <<~END
      ptr_var = new VoidPObject {
          nullptr,
          [&self](auto p) {
              Env e {};
              GlobalEnv::the()->global_set(&e, "$gc_ran"_s, TrueObject::the());
          }
      };
    END
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
      1000.times { Object.new }
      $gc_ran.should == false

      GC.enable
      1000.times { Object.new }
      $gc_ran.should == true
    end
  end
end
