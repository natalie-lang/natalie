# -*- encoding: utf-8 -*-

require_relative '../spec_helper'

describe 'define a method' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can define a method without arguments' do
    code = <<~RUBY
      def foo = 'foo'
      puts foo
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "foo\n"
  end

  it 'can define a method with required positional arguments' do
    code = <<~RUBY
      def foo(a, b) = a + b
      puts foo('bar', 'baz')
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "barbaz\n"
  end

  it 'can define a method with optional arguments' do
    code = <<~RUBY
      def foo(a, b = 'baz') = a + b
      puts foo('bar')
    RUBY

    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "barbaz\n"
  end

  it 'can define a method with required keyword arguments' do
    code = <<~RUBY
      def foo(a, b:) = a + b
      puts foo('bar', b: 'baz')
    RUBY

    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "barbaz\n"
  end

  it 'can define a method with optional keyword arguments' do
    code = <<~RUBY
      def foo(a, b: 'baz') = a + b
      puts foo('bar')
    RUBY

    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "barbaz\n"
  end
end
