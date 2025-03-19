require 'concurrent'
require 'etc'

Thread.abort_on_exception = true

out_dir = ARGV.first
raise 'must specify output directory as first argument' unless out_dir
raise "directory #{out_dir} does not exist" unless File.directory?(out_dir)

pool = Concurrent::ThreadPoolExecutor.new(
  max_threads: Etc.nprocessors,
  max_queue: 0 # unbounded work queue
)

specs = ENV.fetch('GLOB', 'spec/**/*_spec.rb')
env = { 'DO_NOT_PRINT_CPP_PATH' => 'true' }

errors = []

Dir[specs].each do |path|
  pool.post do
    begin
      output_path = File.join(out_dir, path.tr('/.', '_')) + '.cpp'
      system(
        env,
        "bin/natalie -d cpp #{path} 2>/dev/null > #{output_path}",
        exception: true
      )
    rescue Exception => e
      errors << e.message
    end
  end
end

last_count = 0
loop do
  puts "#{pool.queue_length} job(s) remaining" if pool.queue_length != last_count
  last_count = pool.queue_length

  if errors.any?
    puts errors
    exit 1
  end

  break if last_count.zero?

  sleep 1
end

pool.shutdown
pool.wait_for_termination
