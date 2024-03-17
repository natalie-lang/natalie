require_relative '../spec_helper'

describe 'it can run some code with if instructions' do
  before :each do
    @bytecode_file = tmp('bytecode')
  end

  after :each do
    rm_r @bytecode_file
  end

  it 'can run some code with an if in different variations' do
    code = <<~RUBY
      if :foo
        puts "Foo"
      end
      puts "Bar" if nil
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "Foo\n"
  end

  it 'can run some code with an if and an else' do
    code = <<~RUBY
      if :foo
        puts "Foo"
      else
        puts "Bar"
      end
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "Foo\n"
  end

  it 'can run some code with an elsif' do
    code = <<~RUBY
      if false
        puts "Foo"
      elsif :bar
        puts "Bar"
      else
        puts "Baz"
      end
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "Bar\n"
  end

  it 'can use a ternary operator' do
    code = 'puts :foo ? "Foo" : "Bar"'
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "Foo\n"
  end
end
