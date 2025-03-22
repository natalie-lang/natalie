# -*- encoding: utf-8 -*-

require_relative '../spec_helper'

describe 'support multiple assignment' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'supports multiple assignment' do
    code = <<~RUBY
      a, *b, c = [1, 2, 3, 4]
      p [a, b, c]
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: '--bytecode').should == ruby_exe(code)
  end
end
