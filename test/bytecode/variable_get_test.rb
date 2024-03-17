require_relative '../spec_helper'

describe 'it can run some code with a variable_get instruction' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run some code with a variable_get instruction' do
    code = 'puts ["a", "0"].map { |x| x.ord }'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "97\n48\n"
  end
end
