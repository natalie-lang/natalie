require 'minitest/spec'
require 'minitest/autorun'
require 'time'
require 'timeout'

describe 'ruby/spec' do
  parallelize_me!

  PROCESS_TIMEOUT = 120

  Dir.chdir File.expand_path('../..', __dir__)
  Dir['spec/**/*_spec.rb'].each do |path|
    code = File.read(path, encoding: 'utf-8')
    describe path do
      it 'passes all specs' do
        skip if code =~ /# skip-test/
        out_nat = Timeout.timeout(PROCESS_TIMEOUT, nil, "execution expired running: #{path}") do
          `bin/natalie #{path} 2>&1`
        end
        puts out_nat unless $?.success?
        expect($?).must_be :success?
        expect(out_nat).wont_match(/traceback|error/i)
      end
    end
  end
end
