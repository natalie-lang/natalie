require 'minitest/spec'
require 'minitest/autorun'
require 'time'
require 'timeout'
require_relative '../support/nat_binary'

describe 'ruby/spec' do
  parallelize_me!

  SPEC_TIMEOUT = (ENV['SPEC_TIMEOUT'] || 120).to_i

  Dir.chdir File.expand_path('../..', __dir__)
  glob = if ENV['DEBUG_COMPILER']
           # I use this when I'm working on the compiler,
           # as it catches 99% of bugs and finishes a lot quicker.
           Dir['spec/language/*_spec.rb']
         else
           Dir['spec/**/*_spec.rb']
         end
  glob.each do |path|
    describe path do
      it 'passes all specs' do
        obtain_lock if serialize_test?(path)
        out_nat = Timeout.timeout(SPEC_TIMEOUT, nil, "execution expired running: #{path}") { `#{NAT_BINARY} #{path} 2>&1` }
        puts out_nat unless $?.success?
        expect($?).must_be :success?
        expect(out_nat).wont_match(/traceback|error/i)
      ensure
        release_lock
      end
    end
  end

  # NATFIXME: Some of our specs don't like to run simultaneously. :-(
  def serialize_test?(path)
    %w[
      spec/core/io
      spec/library/socket
    ].any? { |dir| path.start_with?(dir) }
  end

  def obtain_lock
    path = File.expand_path('../tmp/serialized_tests_lock.txt', __dir__)
    @file_lock = File.open(path, File::CREAT)
    @file_lock.flock(File::LOCK_EX)
  end

  def release_lock
    return unless @file_lock

    @file_lock.flock(File::LOCK_UN)
    @file_lock.close
    @file_lock = nil
  end
end
