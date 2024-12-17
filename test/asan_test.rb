# Prior to running this test, you should build Natalie with:
#
#     rake clean build_asan
#     ruby test/asan_test.rb

require 'minitest/spec'
require 'minitest/autorun'
require_relative 'support/compare_rubies'

describe 'ASAN tests' do
  include CompareRubies

  Dir.chdir File.expand_path('..', __dir__)

  describe 'examples/hello.rb' do
    it 'it runs without warnings' do
      out = run_nat('examples/hello.rb')
      expect(out).wont_match(/ASan/)
    end
  end

  describe 'examples/fib.rb' do
    it 'it runs without warnings' do
      out = run_nat('examples/fib.rb')
      expect(out).wont_match(/ASan/)
    end
  end
end
