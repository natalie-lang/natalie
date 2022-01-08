require 'minitest/spec'
require 'minitest/autorun'
require 'time'
require 'timeout'
require_relative '../support/nat_binary'

describe 'ruby/spec' do
  parallelize_me!

  SPEC_TIMEOUT = (ENV['SPEC_TIMEOUT'] || 120).to_i

  Dir.chdir File.expand_path('../..', __dir__)
  Dir['spec/**/*_spec.rb'].each do |path|
    code = File.read(path, encoding: 'utf-8')
    describe path do
      it 'passes all specs' do
        skip if code =~ /# skip-test/
        out_nat =
          Timeout.timeout(SPEC_TIMEOUT, nil, "execution expired running: #{path}") { `#{NAT_BINARY} #{path} 2>&1` }
        puts out_nat unless $?.success?
        expect($?).must_be :success?
        expect(out_nat).wont_match(/traceback|error/i)
      end
    end
  end
end
