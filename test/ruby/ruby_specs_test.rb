require 'minitest/spec'
require 'minitest/autorun'
require 'time'
require 'timeout'
require_relative '../support/nat_binary'

describe 'ruby/spec' do
  parallelize_me!

  def spec_timeout(path)
    case path
    when %r{core/(thread|conditionvariable)}
      480
    else
      120
    end
  end

  Dir.chdir File.expand_path('../..', __dir__)
  files =
    if ENV['DEBUG_COMPILER']
      # I use this when I'm working on the compiler,
      # as it catches 99% of bugs and finishes a lot quicker.
      Dir['spec/language/*_spec.rb']
    else
      Dir['spec/**/*_spec.rb']
    end

  if !(glob = ENV['GLOB']).to_s.empty?
    # GLOB="spec/core/io/*_spec.rb,spec/core/thread/*_spec.rb" rake test
    glob_files = files & Dir[*glob.split(',')]
    puts "Matched #{glob_files.size} out of #{files.size} total spec files:"
    puts glob_files.to_a
    files = glob_files
  end

  files.each do |path|
    describe path do
      it 'passes all specs' do
        out_nat =
          Timeout.timeout(spec_timeout(path), nil, "execution expired running: #{path}") do
            `#{NAT_BINARY} --build-dir=test/build --build-quietly #{path} 2>&1`
          end
        puts out_nat if ENV['DEBUG'] || !$?.success?
        expect($?).must_be :success?
        expect(out_nat).wont_match(/traceback|error/i)
      end
    end
  end
end
