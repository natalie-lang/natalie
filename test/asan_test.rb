# Prior to running this test, you should build Natalie with:
#
#     rake clean build_asan
#     ruby test/asan_test.rb

require 'fileutils'
require 'minitest/spec'
require 'minitest/autorun'
require_relative 'support/compare_rubies'

TESTS = Dir[
  'test/natalie/**/*_test.rb',
  'spec/language/*_spec.rb',
].to_a

TESTS_TO_SKIP = [
  'test/natalie/ffi_test.rb', # memory leak
  'test/natalie/libnat_test.rb', # too slow, times out frequently
  'test/natalie/thread_test.rb', # calls GC.start, but we're not ready for that
].freeze

describe 'ASAN tests' do
  include CompareRubies

  parallelize_me!

  Dir.chdir File.expand_path('..', __dir__)

  before do
    FileUtils.mkdir_p 'test/tmp'
  end

  TESTS.each do |path|
    next if TESTS_TO_SKIP.include?(path)

    describe path do
      it 'it runs without warnings' do
        out = run_nat(path)
        expect(out).wont_match(/ASan/)
      end
    end
  end

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

  describe 'examples/boardslam.rb' do
    it 'it runs without warnings' do
      out = run_nat('examples/boardslam.rb', 3, 5, 1)
      expect(out).wont_match(/ASan/)
    end
  end
end
