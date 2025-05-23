require 'minitest/spec'
require 'minitest/autorun'
require 'time'
require_relative '../support/nat_binary'
require_relative '../support/command'

describe 'ruby/spec' do
  parallelize_me!

  def spec_timeout(path)
    return ENV['SPEC_TIMEOUT'].to_i if ENV['SPEC_TIMEOUT']

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
        timeout = spec_timeout(path)
        out = Command.new(NAT_BINARY, '--build-dir=test/build', '--build-quietly', path, timeout:).run
        puts out if ENV['DEBUG']
        expect(out).wont_match(/traceback|error/i)
      end
    end
  end
end
