# Prior to running this test, you should build Natalie with:
#
#     rake clean build_asan
#     ruby test/asan_test.rb

require 'minitest/spec'
require 'minitest/autorun'
require_relative 'support/compare_rubies'

TEST_GLOBS = [
  'test/natalie/**/*_test.rb',
].freeze

# FIXME: These tests fail, crash, or produce warnings from the AddressSanitizer.
SKIP_TESTS = %w[
  test/natalie/array_test.rb
  test/natalie/dir_test.rb
  test/natalie/encoding_test.rb
  test/natalie/enumerator_test.rb
  test/natalie/enum_for_with_keyword_arguments_test.rb
  test/natalie/equality_test.rb
  test/natalie/ffi_test.rb
  test/natalie/fiber/scheduler_sleep_test.rb
  test/natalie/fiber_test.rb
  test/natalie/integer_test.rb
  test/natalie/kernel_test.rb
  test/natalie/libnat_test.rb
  test/natalie/loop_test.rb
  test/natalie/module_test.rb
  test/natalie/openssl_test.rb
  test/natalie/optparse_test.rb
  test/natalie/pp_test.rb
  test/natalie/require_test.rb
  test/natalie/reverse_each_test.rb
  test/natalie/singleton_class_test.rb
  test/natalie/stringio_test.rb
  test/natalie/string_test.rb
  test/natalie/struct_test.rb
  test/natalie/thread_test.rb
].freeze

describe 'ASAN tests' do
  include CompareRubies

  parallelize_me!

  Dir.chdir File.expand_path('..', __dir__)
  Dir[*TEST_GLOBS].each do |path|
    next if SKIP_TESTS.include?(path)

    code = File.read(path, encoding: 'utf-8')
    describe path do
      if code =~ /# skip-ruby/
        it 'it passes' do
          run_nat(path)
        end
      else
        it 'has the same output in ruby and natalie' do
          run_both_and_compare(path)
        end
      end
    end
  end

  {
    'examples/hello.rb' => [],
    'examples/fib.rb'   => [],
    #'examples/boardslam.rb' => [3, 5, 1], # FIXME: produces ASAN warning :-(
  }.each do |path, args|
    describe path do
      it 'has the same output in ruby and natalie' do
        run_both_and_compare(path, *args)
      end
    end
  end
end
