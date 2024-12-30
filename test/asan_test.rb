# Prior to running this test, you should build Natalie with:
#
#     rake clean build_sanitized
#     ruby test/asan_test.rb

require 'fileutils'
require 'minitest/spec'
require 'minitest/autorun'
require 'minitest/reporters'
require_relative 'support/compare_rubies'

case ENV['REPORTER']
when 'spec'
  Minitest::Reporters.use!(Minitest::Reporters::SpecReporter.new)
when 'progress', nil
  Minitest::Reporters.use!(Minitest::Reporters::ProgressReporter.new(detailed_skip: false))
when 'dots'
  # just use the default reporter
end

TESTS = if ENV['SOME_TESTS'] == 'true'
          # runs on every PR -- some tests
          Dir[
            'spec/language/*_spec.rb',
            'test/natalie/**/*_test.rb',
            # fixed:
            'spec/core/kernel/Float_spec.rb',
            'spec/core/kernel/srand_spec.rb',
            'spec/core/process/spawn_spec.rb',
            'spec/core/random/new_seed_spec.rb',
            'spec/core/random/srand_spec.rb',
            'spec/core/string/crypt_spec.rb',
            'spec/library/yaml/dump_spec.rb',
            'spec/library/yaml/load_spec.rb',
            'spec/library/yaml/to_yaml_spec.rb',
          ].to_a
        else
          # runs nightly -- all tests
          Dir[
            'spec/**/*_spec.rb',
            'test/natalie/**/*_test.rb',
          ].to_a
        end

TESTS_TO_SKIP = [
  # calls GC.start/GC.enable, but we're not ready for that
  'spec/core/gc/disable_spec.rb',
  'spec/core/gc/enable_spec.rb',
  'test/natalie/gc_test.rb',
  'test/natalie/thread_test.rb',

  # leak after use of PEM_read_bio_PUBKEY
  'test/natalie/openssl_test.rb',

  # getaddrinfo "leak"
  # https://bugs.kde.org/show_bug.cgi?id=448991
  # https://bugzilla.redhat.com/show_bug.cgi?id=859717
  # My understanding is that it is a single object that is internal to glibc and never freed.
  'spec/library/socket/basicsocket/do_not_reverse_lookup_spec.rb',
  'spec/library/socket/ipsocket/addr_spec.rb',
  'spec/library/socket/ipsocket/getaddress_spec.rb',
  'spec/library/socket/ipsocket/peeraddr_spec.rb',
  'spec/library/socket/ipsocket/recvfrom_spec.rb',
  'spec/library/socket/socket/getaddrinfo_spec.rb',
  'spec/library/socket/tcpserver/new_spec.rb',
  'spec/library/socket/tcpserver/sysaccept_spec.rb',
  'spec/library/socket/tcpsocket/initialize_spec.rb',
  'spec/library/socket/tcpsocket/open_spec.rb',
  'spec/library/socket/tcpsocket/recv_nonblock_spec.rb',
  'spec/library/socket/tcpsocket/setsockopt_spec.rb',
  'spec/library/socket/udpsocket/bind_spec.rb',
  'spec/library/socket/udpsocket/write_spec.rb',

  # spec timeout, hangs on waitpid
  'spec/core/kernel/fork_spec.rb',
  'spec/core/process/fork_spec.rb',

   # some issue to do with ptrace + Docker privileges
  'spec/core/process/egid_spec.rb',
  'spec/core/process/euid_spec.rb',
  'spec/core/process/uid_spec.rb',
].freeze

describe 'Sanitizers tests' do
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
      expect(out).wont_match(/ASan|traceback|error/)
    end
  end

  describe 'examples/boardslam.rb' do
    it 'it runs without warnings' do
      out = run_nat('examples/boardslam.rb', 3, 5, 1)
      expect(out).wont_match(/ASan|traceback|error/)
    end
  end

  describe 'bin/nat' do
    it 'it runs without warnings' do
      out = sh('bin/nat -c /tmp/bs examples/boardslam.rb')
      puts out unless $?.success?
      expect($?).must_be :success?
      expect(out).wont_match(/ASan|traceback|error/)
    end
  end
end
