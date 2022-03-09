require 'minitest/spec'
require 'minitest/autorun'
require 'time'
require 'timeout'
require_relative '../support/nat_binary'

describe 'ruby/spec' do
  parallelize_me!

  PER_RUN = 10
  SPEC_TIMEOUT = (ENV['SPEC_TIMEOUT'] || 600).to_i

  Dir.chdir File.expand_path('../..', __dir__)
  paths = Dir['spec/**/*_spec.rb'].sort
  skipped_tests, tests = paths.partition { |path| File.read(path, encoding: 'utf-8') =~ /# skip-test/ }

  tests.each_slice(PER_RUN) do |paths|
    describe paths.join(' ') do
      it 'passes all specs' do
        path_args = paths.map { |p| "-r #{File.expand_path(p)}" }
        out_nat = Timeout.timeout(SPEC_TIMEOUT, nil, "execution expired running: #{paths.join(', ')}") { `#{NAT_BINARY} #{path_args.join(' ')} -e '' 2>&1` }
        puts out_nat unless $?.success?
        expect($?).must_be :success?
        expect(out_nat).wont_match(/traceback|error/i)
      end
    end
  end

  skipped_tests.each do |path|
    describe path do
      it 'passes all specs' do
        skip
      end
    end
  end
end
