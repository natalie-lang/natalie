require 'minitest/spec'
require 'minitest/autorun'

begin
  $LOAD_PATH << File.expand_path('../../build/natalie_parser/lib', __dir__)
  $LOAD_PATH << File.expand_path('../../build/natalie_parser/ext', __dir__)
  require 'natalie_parser'
rescue LoadError
  puts 'Error: You must build natalie_parser.so by running: rake parser_c_ext'
  exit 1
end

describe 'Natalie Parser C Extension' do
  parallelize_me!

  it 'works with an instance' do
    expect(NatalieParser.new('1', '-e').parse).must_equal(s(:block, s(:lit, 1)))
  end

  it 'works using the class method' do
    expect(NatalieParser.parse('1', '-e')).must_equal(s(:block, s(:lit, 1)))
  end

  it 'works via the command line' do
    result = `bin/natalie --nat-parser --ast -e ":foo"`.strip
    expect(result).must_equal 's(:block, s(:lit, :foo))'
  end
end
