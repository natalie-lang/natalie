# -*- encoding: utf-8 -*-

require_relative '../spec_helper'

describe 'puts a string' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run a puts with a static string' do
    code = 'puts "foo"'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: '--bytecode').should == ruby_exe(code)
  end

  it 'saves the original encoding' do
    code = <<~RUBY
      # -*- encoding: utf-8 -*-

      puts "😊".encoding
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: '--bytecode').should == ruby_exe(code)
  end

  it 'saves the original encoding (2)' do
    code = <<~RUBY
      # -*- encoding: binary -*-

      puts "😊".encoding
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: '--bytecode').should == ruby_exe(code)
  end

  it 'supports string interpolation' do
    code = 'puts "foo #{120 + 3}"'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: '--bytecode').should == ruby_exe(code)
  end
end
