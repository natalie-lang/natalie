# Prior to running this test, you should build Natalie with:
#
#     rake clean build_asan
#     ruby test/asan_test.rb

require 'fileutils'
require 'minitest/spec'
require 'minitest/autorun'
require_relative 'support/compare_rubies'

TESTS = if ENV['SOME_TESTS'] == 'true'
          # runs on every PR -- some tests
          Dir[
            'spec/language/*_spec.rb',
            'test/natalie/**/*_test.rb',
          ].to_a
        else
          # runs nightly -- all tests
          Dir[
            'spec/**/*_spec.rb',
            'test/natalie/**/*_test.rb',
          ].to_a
        end

TESTS_TO_SKIP = [
  'test/natalie/libnat_test.rb', # too slow, times out frequently
  'test/natalie/thread_test.rb', # calls GC.start, but we're not ready for that
  'spec/library/socket/basicsocket/do_not_reverse_lookup_spec.rb', # getaddrinfo leak
  'spec/library/socket/ipsocket/getaddress_spec.rb', # getaddrinfo leak
  'spec/library/socket/socket/getaddrinfo_spec.rb', # getaddrinfo leak
  'spec/library/socket/tcpsocket/initialize_spec.rb', # getaddrinfo leak
  'spec/library/socket/tcpserver/new_spec.rb', # getaddrinfo leak
  'spec/library/socket/tcpserver/sysaccept_spec.rb', # getaddrinfo leak
  'spec/library/socket/tcpsocket/setsockopt_spec.rb', # getaddrinfo leak
  'spec/library/socket/ipsocket/peeraddr_spec.rb', # getaddrinfo leak
  'spec/library/socket/tcpsocket/recv_nonblock_spec.rb', # getaddrinfo leak
  'spec/library/socket/udpsocket/bind_spec.rb', # getaddrinfo leak
  'spec/library/socket/tcpsocket/open_spec.rb', # getaddrinfo leak
  'spec/library/socket/udpsocket/write_spec.rb', # getaddrinfo leak
  'spec/library/socket/ipsocket/recvfrom_spec.rb', # getaddrinfo leak
  'spec/library/socket/ipsocket/addr_spec.rb', # getaddrinfo leak
  'spec/library/yaml/unsafe_load_spec.rb', # heap buffer overflow in Natalie::StringObject::convert_float()
  'spec/library/yaml/load_spec.rb', # heap buffer overflow in Natalie::StringObject::convert_float()
  'spec/library/yaml/dump_spec.rb', # heap buffer overflow in Natalie::StringObject::convert_float()
  'spec/library/yaml/to_yaml_spec.rb', # heap buffer overflow in Natalie::StringObject::convert_float()
  'spec/core/process/spawn_spec.rb', # leak in KernelModule::spawn
  'spec/core/process/fork_spec.rb', # spec timeout
  'spec/core/kernel/fork_spec.rb', # spec timeout
  'spec/core/kernel/Float_spec.rb', # heap buffer overflow in Natalie::StringObject::convert_float()
  'spec/core/kernel/srand_spec.rb', # leak in Natalie::RandomObject::srand
  'spec/core/random/new_seed_spec.rb', # leak in Natalie::RandomObject::srand
  'spec/core/random/srand_spec.rb', # leak in Natalie::RandomObject::srand
  'spec/core/process/uid_spec.rb', # not sure why this breaks
  'spec/core/process/euid_spec.rb', # not sure why this breaks
  'spec/core/process/egid_spec.rb', # not sure why this breaks
  'spec/core/string/crypt_spec.rb', # heap buffer overflow in Natalie::StringObject::crypt
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
