# Run more than one test at a time.
#
# usage:
#
#     bin/natalie test/runner.rb spec/core/array/pack/{a,b}_spec.rb
#
# to run all the tests grouped:
#
#     bin/natalie test/runner.rb --run-grouped spec/core/array/pack/{a,b}_spec.rb
#
# usage with flags:
#
#     bin/natalie -c2 test/runner.rb -- -c2 spec/core/array/pack/{a,b}_spec.rb

require_relative 'support/nat_binary'

flags = []
flags << ARGV.shift while ARGV.first.to_s.start_with?('-')

if flags.delete('--run-grouped')
  flags.prepend('-I.', '-Itest/support')
  require 'tempfile'
  Tempfile.create('test_runner') do |f|
    ARGV.each do |path|
      if File.directory?(path)
        $stderr.puts "WARNING: skipping directory #{path}"
        next
      elsif !File.exist?(path)
        $stderr.puts "WARNING: skipping invalid file #{path}"
        next
      end

      f.puts "require #{path.inspect}"
    end
    f.close

    pid = spawn(NAT_BINARY, *(flags + [f.path]))
    Process.wait(pid)
  end
  exit $?.exitstatus unless $?.success?
else
  ARGV.each do |path|
    if File.directory?(path)
      $stderr.puts "WARNING: skipping directory #{path}"
      next
    end
    puts path
    pid = spawn(NAT_BINARY, *(flags + [path]))
    Process.wait(pid)
    exit $?.exitstatus unless $?.success?
  end
end
