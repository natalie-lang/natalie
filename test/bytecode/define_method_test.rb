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

  it 'can define a method with splat positional arguments' do
    code = <<~RUBY
      def foo(*args) = args.join
      puts foo('bar', 'baz')
    RUBY

    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "barbaz\n"
  end

  it 'can define a method with splat keyword arguments' do
    code = <<~RUBY
      def foo(**kwargs) = kwargs.keys.join
      puts foo(bar: 'bar', baz: 'quux')
    RUBY

    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "barbaz\n"
  end

  it 'can define a method with splat positional and keyword arguments' do
    code = <<~RUBY
      def foo(*args, **kwargs) = (args + kwargs.keys).join
      puts foo('bar', baz: 'quux')
    RUBY

    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "barbaz\n"
  end

  it 'can handle a block argument' do
    code = <<~RUBY
      def foo(&block) = block.nil?
      puts foo { 1 }
      puts foo
    RUBY

    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "false\ntrue\n"
  end

  it 'can define a method with forwarding for everything (...)' do
    code = <<~RUBY
      def foo(*args, **kwargs) = (args + kwargs.keys).join
      def bar(...) = foo(...)
      puts bar('bar', baz: 'quux')
    RUBY

    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "barbaz\n"
  end
end
