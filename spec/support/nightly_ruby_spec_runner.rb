# This file is used to run the whole ruby/spec suite against Natalie. The results are
# published on natalie-lang.org.
#
# Do not use this script to run specs locally.

require 'concurrent'
require 'yaml'
require 'json'
require 'io/wait'
require 'uri'
require 'net/http'

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
details = {}

specs = 'spec/**/*_spec.rb'

def recursive_sort(hash)
  hash.keys.sort.reduce({}) do |seed, key|
    seed[key] = hash[key]
    if seed[key].is_a?(Hash)
      seed[key] = recursive_sort(seed[key])
    end
    seed
  end
end

puts "Start running specs #{specs}"

Dir[specs].each do |path|
  pool.post {
    binary_name = path[..-4]

    splitted_path = path.split('/')
    current = details
    splitted_path.each do |part|
      current = current[part] ||= {}
    end

    current.merge!(
      compiled: false,
      timeouted: false,
      crashed: false,
      success: 0,
      failures: 0,
      errors: 0,
      error_messages: []
    )

    unless system("bin/natalie -c #{binary_name} #{path} 2> /dev/null")
      compile_errors += 1
      puts "Spec #{path} could not be compiled"
      return
    end

    current[:compiled] = true

    IO.popen(["./#{binary_name}", "-f", "yaml"], err: '/dev/null') { |f|
      if f.wait_readable(180)
        yaml = YAML.safe_load(f.read) || {}
        # If one of those is not given there is something wrong... (probably crashed)
        if yaml["examples"] && yaml["errors"] && yaml["failures"]
          current.merge!(
            success: yaml["examples"] - yaml["errors"] - yaml["failures"],
            failures: yaml["failures"],
            errors: yaml["errors"],
            error_messages: yaml["exceptions"] || []
          )

          success_count += current[:success]
          failure_count += current[:failures]
          error_count += current[:errors]

          puts "Ran spec #{path}"
        end
      else
        puts "Spec #{path} timed out (after 3 seconds)"
        current[:timeouted] = true
        timed_out_count += 1
        Process.kill('TERM', f.pid)
      end
    }

    File.unlink(binary_name)

    status = $?
    # If the process did not exit normally it was probably shut down by the
    # `Process.kill` call when timeouting a spec
    if !status.success? && !current[:timeouted]
      puts "Spec #{path} crashed"
      current[:crashed] = true
      crashed_count += 1
    end

  }
end
pool.shutdown
pool.wait_for_termination

stats = {
  "Successful Examples" => success_count,
  "Failed Examples" => failure_count,
  "Errored Examples" => error_count,
  "Compile Errors" => compile_errors,
  "Crashes" => crashed_count,
  "Timeouts" => timed_out_count,
  "Details" => recursive_sort(details)
}
p stats.reject { |k, _| k == "Details" }

uri = URI('https://stats.natalie-lang.org/stats')
p uri
https = Net::HTTP.new(uri.host, uri.port)
https.use_ssl = true
p https.post(
  uri.path,
  URI.encode_www_form(
    'stats' => stats.to_json,
    'secret' => ENV['STATS_API_SECRET']
  )
)
