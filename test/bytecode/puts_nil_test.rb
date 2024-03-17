require_relative '../spec_helper'

describe 'puts a nil' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run a puts with nil' do
    code = 'puts nil.inspect'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "nil\n"
  end
end
