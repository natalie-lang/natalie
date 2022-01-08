require 'minitest/spec'
require 'minitest/autorun'
require_relative '../support/nat_binary'

describe 'RbConfig' do
  describe 'CONFIG' do
    it 'has a bindir that points to natalie' do
      out = `#{NAT_BINARY} -r rbconfig -e "puts RbConfig::CONFIG[:bindir]"`.strip
      expect(out).must_equal File.expand_path('../../bin', __dir__)
    end
  end
end
