require_relative '../spec_helper'

describe 'puts an array' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run a puts with an array with static strings' do
    code = 'puts ["foo", "bar"]'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: '--bytecode').should == ruby_exe(code)
  end

  it 'can run a puts with a string representation of an array with static strings' do
    code = 'puts ["foo", "bar"].to_s'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: '--bytecode').should == ruby_exe(code)
  end
end
