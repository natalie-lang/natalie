require 'minitest/spec'
require 'minitest/autorun'
require_relative '../support/nat_binary'

describe 'special globals' do
  parallelize_me!

  describe '$0' do
    it 'returns the path to the initial ruby script executed/compiled' do
      expect(`#{NAT_BINARY} test/support/dollar_zero.rb`.strip).must_equal '"test/support/dollar_zero.rb"'
      `#{NAT_BINARY} -c test/tmp/dollar_zero test/support/dollar_zero.rb`
      expect(`test/tmp/dollar_zero`.strip).must_equal '"test/support/dollar_zero.rb"'
    end
  end

  describe '$exe' do
    it 'returns the path to the compiled binary exe' do
      expect(`#{NAT_BINARY} test/support/dollar_exe.rb`.strip).must_match /natalie/ # usually something like: /tmp/natalie20210119-2242842-eystnh
      `#{NAT_BINARY} -c test/tmp/dollar_exe test/support/dollar_exe.rb`
      expect(`test/tmp/dollar_exe`.strip).must_equal '"test/tmp/dollar_exe"'
    end
  end
end
