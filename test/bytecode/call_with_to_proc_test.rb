require_relative '../spec_helper'

describe 'call a method with a to_proc argument' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can call a method with a to_proc argument' do
    code = 'p [1, 2].map(&:to_s)'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "[\"1\", \"2\"]\n"
  end
end
