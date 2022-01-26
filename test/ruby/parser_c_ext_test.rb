require 'minitest/spec'
require 'minitest/autorun'

describe 'Natalie Parser C Extension' do
  parallelize_me!

  it 'works' do
    result = `bin/natalie --nat-parser --ast -e ":foo"`.strip
    expect(result).must_equal 's(:block, s(:lit, :foo))'
  end
end
