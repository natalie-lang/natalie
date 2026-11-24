require 'etc'
require 'rbconfig'
require_relative '../lib/thread_pool'

Thread.abort_on_exception = true

out_dir = ARGV.fetch(0)
raise "directory #{out_dir} does not exist" unless File.directory?(out_dir)

pool = ThreadPool.new([Etc.nprocessors, 4].min)
files = Dir[ENV.fetch('GLOB', 'spec/**/*_spec.rb')]
env = { 'DO_NOT_PRINT_SOURCE_PATH' => 'true' }
errors = []
lock = Mutex.new

files.each do |path|
  pool.schedule do
    output_path = File.join(out_dir, path.tr('/.', '_')) + '.source'
    begin
      system(env, RbConfig.ruby, 'bin/natalie', '-d', 'code', path, out: output_path, exception: true)
    rescue StandardError => error
      lock.synchronize { errors << error.message }
    end
  end
end

pool.shutdown
abort errors.join("\n") unless errors.empty?
puts "Generated #{files.size} source file(s)"
