require_relative '../spec_helper'

describe 'it can run some code with a variable_declare instruction' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run some code with a variable_declar instruction' do
    code = 'a = 1; puts a'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "1\n"
  end
end
