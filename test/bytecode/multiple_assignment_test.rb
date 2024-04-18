# -*- encoding: utf-8 -*-

require_relative '../spec_helper'

# `monkey.rb` from https://ruby-compilers.com/examples/
describe 'support monkey patching' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'supports monkey patching' do
    code = <<~RUBY
      a, *b, c = [1, 2, 3, 4]
      p [a, b, c]
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "[1, [2, 3], 4]\n"
  end
end
