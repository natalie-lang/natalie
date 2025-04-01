require 'minitest/spec'
require 'minitest/autorun'
require 'time'
require 'timeout'
require_relative '../support/nat_binary'
require 'open3'

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
        out_nat = []
        status = -1
        begin
          Timeout.timeout(spec_timeout(path), nil, "execution expired running: #{path}") do
            status =
              Open3.popen2e(NAT_BINARY, '--build-dir=test/build', '--build-quietly', path) do |_i, o, t|
                begin
                  out_nat << o.gets until o.eof?
                  t.value.to_i
                ensure
                  begin
                    Process.kill(9, t.pid)
                  rescue StandardError
                    nil
                  end
                end
              end
          end
        ensure
          puts out_nat.join("\n") if ENV['DEBUG'] || status != 0
        end
        expect(status).must_equal(0)
        expect(out_nat.join("\n")).wont_match(/traceback|error/i)
      end
    end
  end
end
