# skip-ruby

require_relative '../spec_helper'

describe 'compiler debug output' do
  it 'works for every pass' do
    %w[p1 p2 p3 p4].each do |pass|
      ruby_exe('puts "hello world"', options: "-d #{pass}").should =~ /0 push_self.*send :puts to self/m
    end
  end
end
