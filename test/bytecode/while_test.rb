# -*- encoding: utf-8 -*-

require_relative '../spec_helper'

describe 'run a while loop' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run a simple while loop' do
    code = <<~RUBY
      i = 1
      while i <= 3
        puts i
        i += 1
      end
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "1\n2\n3\n"
  end
end
