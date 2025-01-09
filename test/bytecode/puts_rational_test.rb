require_relative '../spec_helper'

describe 'puts a rational number' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run a puts with a rational number' do
    code = 'puts 2.5r'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == ruby_exe(code)
  end
end
