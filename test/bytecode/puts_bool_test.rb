require_relative '../spec_helper'

describe 'puts a boolean' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run a puts with booleans' do
    code = 'puts true; puts false'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "true\nfalse\n"
  end
end
