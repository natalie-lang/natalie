require 'minitest/spec'
require 'minitest/autorun'

describe 'top-level rescue' do
  parallelize_me!

  it 'works' do
    expect(`bin/natalie -e "begin; raise 'foo'; rescue; end; puts 'works'"`.strip).must_equal 'works'
  end
end
