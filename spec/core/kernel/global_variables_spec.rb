require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "Kernel.global_variables" do
  it "is a private method" do
    Kernel.should have_private_instance_method(:global_variables)
  end

  before :all do
    @i = 0
  end

  it "finds subset starting with std" do
    global_variables.grep(/std/).should include(:$stderr, :$stdin, :$stdout)
    a = global_variables.size
    gvar_name = "$foolish_global_var#{@i += 1}"
    global_variables.include?(gvar_name.to_sym).should == false
    NATFIXME 'eval', exception: TypeError, message: 'eval() only works on static strings' do
      eval("#{gvar_name} = 1")
      global_variables.size.should == a+1
      global_variables.should include(gvar_name.to_sym)
    end
  end
end

describe "Kernel#global_variables" do
  it "needs to be reviewed for spec completeness"
end
