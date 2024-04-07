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
      class Integer
        def *(other)
          self + other
        end
      end

      def multiply(a, b)
        a * b
      end

      puts multiply(14, 2)
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "16\n"
  end
end
