# -*- encoding: utf-8 -*-

require_relative '../spec_helper'

describe 'puts a regexp match result' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run a puts with a regexp match result' do
    code = 'puts /(foo)/.match("foobar")'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "foo\n"
  end

  it 'preserves the regexp options' do
    code = <<~RUBY
      r = /foo/i
      puts r.options
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "#{Regexp::IGNORECASE}\n"
  end
end
