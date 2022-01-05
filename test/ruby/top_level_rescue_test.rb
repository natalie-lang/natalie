require 'minitest/spec'
require 'minitest/autorun'
require_relative '../support/nat_binary'

describe 'top-level rescue' do
  parallelize_me!

  it 'works' do
    expect(`#{NAT_BINARY} -e "begin; raise 'foo'; rescue; end; puts 'works'"`.strip).must_equal 'works'
  end
end
