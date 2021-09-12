require 'concurrent'
require 'yaml'
require 'io/wait'

pool = Concurrent::ThreadPoolExecutor.new(
  max_threads: 4,
  max_queue: 0 # unbounded work queue
)
success_count = 0
failure_count = 0
error_count = 0
compile_errors = 0
crashed_count = 0
timed_out_count = 0

specs = 'spec/**/*_spec.rb'

puts "Start running specs #{specs}"

Dir[specs].each do |path|
  pool.post {
    binary_name = path[..-4]
    unless system("bin/natalie #{path} -c #{binary_name} 2> /dev/null")
      compile_errors += 1
      puts "Spec #{path} could not be compiled"
      return
    end

    IO.popen(["./#{binary_name}", "-f", "yaml"], err: '/dev/null') { |f|
      if f.wait_readable(180)
        yaml = YAML.safe_load(f.read) || {}
        # If one of those is not given there is something wrong... (probably crashed)
        if yaml["examples"] && yaml["errors"] && yaml["failures"]
          success_count += yaml["examples"] - yaml["errors"] - yaml["failures"]
          failure_count += yaml["failures"]
          error_count += yaml["errors"]
          puts "Ran spec #{path}"
        end
      else
        puts "Spec #{path} timed out (after 3 seconds)"
        timed_out_count += 1
        Process.kill('TERM', f.pid)
      end
    }

    status = $?
    # If the process did not exit normally it was probably shut down by the
    # `Process.kill` call when timeouting a spec
    if status.exited? && !status.success?
      puts "Spec #{path} crashed"
      crashed_count += 1
    end
  }
end
pool.shutdown
pool.wait_for_termination

puts "Success: #{success_count}"
puts "Failures: #{failure_count}"
puts "Errors: #{error_count}"
puts "Compile Errors: #{compile_errors}"
puts "Crashes: #{crashed_count}"
puts "Timeouts: #{timed_out_count}"