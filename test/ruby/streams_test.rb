require 'minitest/spec'
require 'minitest/autorun'
require_relative '../support/nat_binary'

describe 'Streams' do
  parallelize_me!

  describe '$stdin' do
    it 'can be read from' do
      out = `echo "hi" | #{NAT_BINARY} -e "print \\$stdin.read"`
      expect(out).must_equal("hi\n")
      out = `sh -c 'echo "hi" | #{NAT_BINARY} -e "p \\$stdin.read.strip"'`
      expect(out).must_equal("\"hi\"\n")
    end
  end

  describe '$stdout' do
    it 'can be reassigned' do
      out = `#{NAT_BINARY} -e "\\$stdout = File.new('test/tmp/stdout.txt', 'w'); puts 1; p 2; print 3"`
      expect(out).must_equal('')
      expect(File.read('test/tmp/stdout.txt')).must_equal("1\n2\n3")
    end
  end

  describe '$stderr' do
    it 'can be reassigned' do
      out = `#{NAT_BINARY} -e "\\$stderr = File.new('test/tmp/stderr.txt', 'w'); raise 'foo'"`
      expect(out).must_equal('')
      expect(File.read('test/tmp/stderr.txt')).must_match(/foo.*RuntimeError/)
    end
  end
end
