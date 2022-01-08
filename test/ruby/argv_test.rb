require 'minitest/spec'
require 'minitest/autorun'
require_relative '../support/nat_binary'

describe 'ARGV' do
  parallelize_me!

  it 'is an empty array if there are no args' do
    expect(`#{NAT_BINARY} -e "p ARGV"`.strip).must_equal '[]'
  end

  it 'is an array containing args passed to the process' do
    expect(`#{NAT_BINARY} -e "p ARGV" x y`.strip).must_equal '["x", "y"]'
  end

  it 'contains args after the double hyphen --' do
    expect(`#{NAT_BINARY} -e "p ARGV" -h`.strip).must_match /Usage:/
    expect(`#{NAT_BINARY} -e "p ARGV" -- -h`.strip).must_equal '["-h"]'
  end
end
