# skip-ruby

require_relative '../spec_helper'

TMPDIR = ENV.fetch('TMPDIR', '/tmp')

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
    out = ruby_exe('puts "hello world"', options: '-d cc-cmd')
    out.should include(cc)
    out.should include("-o #{TMPDIR}")
  end
end

describe '--keep-cpp' do
  it 'prints the temporary cpp file path' do
    ruby_exe('puts "hello world"', options: '--keep-cpp').should include(File.join(TMPDIR, 'ruby_exe.rb'))
  end
end
