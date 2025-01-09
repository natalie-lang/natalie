require_relative '../spec_helper'

describe 'it can read and write global variables' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'defaults to nil for a global variable' do
    code = 'p $FOO'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == ruby_exe(code)
  end

  it 'can read and write global variables' do
    code = <<~RUBY
      $FOO = 123
      puts $FOO
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == ruby_exe(code)
  end
end
