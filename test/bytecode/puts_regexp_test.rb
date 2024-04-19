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

  it 'can use an interpolated regexp' do
    code = <<~'RUBY'
      puts /(fo#{:o})/.match("foobar")
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "foo\n"
  end

  it 'preserves the regexp options of an interpolated regexp' do
    code = <<~'RUBY'
      r = /fo#{:o}/i
      puts r.options
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "#{Regexp::IGNORECASE}\n"
  end

  it 'updates the $~ variable' do
    code = <<~RUBY
      /(foo)/ =~ 'foobar'
      puts $~
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "foo\n"
  end

  it 'can use $1 to access the first match' do
    code = <<~RUBY
      /(foo)/ =~ 'foobar'
      puts $1
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "foo\n"
  end

  it 'can store matches in local variables' do
    code = <<~RUBY
      /(?<foo>bar)/ =~ 'foobar'
      puts foo
    RUBY
    ruby_exe(code, options: "--compile-bytecode #{@bytecode_file}")

    ruby_exe(@bytecode_file, options: "--bytecode").should == "bar\n"
  end
end
