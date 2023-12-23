require_relative '../spec_helper'
require 'natalie/inline'

$gc_result

describe 'GC' do
  describe '.start' do
    def create_object
      $gc_result = "GC has not yet run"
      ptr = nil
      # Our VoidPObject has a cleanup function that runs whenever
      # the object is collected by the GC, so we can use that to
      # test that it did indeed run.
      __inline__ <<~END
        ptr_var = new VoidPObject {
            nullptr,
            [&self](auto p) {
                Env e {};
                auto result = new StringObject { "object was collected" };
                GlobalEnv::the()->global_set(&e, "$gc_result"_s, result);
            }
        };
      END
    end

    it 'collects unreachable objects' do
      create_object
      $gc_result.should == "GC has not yet run"
      GC.start
      $gc_result.should == "object was collected"
    end
  end
end
