require 'minitest/spec'
require 'minitest/autorun'

begin
  require_relative '../../build/parser_c_ext'
rescue LoadError
  puts 'Error: You must build parser_c_ext.so by running: rake parser_c_ext'
  exit 1
end
require_relative '../../lib/sexp_stub_for_use_by_c_ext'

describe 'Natalie Parser C Extension' do
  parallelize_me!

  it 'works with an instance' do
    expect(Parser.new('1', '-e').parse).must_equal(s(:block, s(:lit, 1)))
  end

  it 'works using the class method' do
    expect(Parser.parse('1', '-e')).must_equal(s(:block, s(:lit, 1)))
  end

  it 'works via the command line' do
    result = `bin/natalie --nat-parser --ast -e ":foo"`.strip
    expect(result).must_equal 's(:block, s(:lit, :foo))'
  end
end
