# skip-ruby

require_relative '../spec_helper'

describe '-d' do
  specify '-d {p1,p2,p3,p4}' do
    %w[p1 p2 p3 p4].each do |pass|
      ruby_exe('puts "hello world"', options: "-d #{pass}").should =~ /0 push_self.*send :puts to self/m
    end
  end

  specify '-d cpp' do
    ruby_exe('puts "hello world"', options: '-d cpp').should include('int main(int argc, char *argv[])')
  end

  specify '-d cc-cmd' do
    cc = ENV['CXX'] || 'c++'
    ruby_exe('puts "hello world"', options: '-d cc-cmd').should =~ /^#{Regexp.escape(cc)}.*\-o temp/m
  end

  specify '-d edit' do
    binary = ENV.fetch('NAT_BINARY', 'bin/natalie')
    out = `echo n | EDITOR=echo #{binary} -d edit examples/hello.rb`
    out.should include('tmp/natalie.cpp')
    out.should include('hello world')
    out.should include('Edit again? [Yn]')
  end
end

describe '--keep-cpp' do
  it 'prints the temporary cpp file path' do
    ruby_exe('puts "hello world"', options: '--keep-cpp').should =~ /tmp\/natalie\.cpp/
  end
end
