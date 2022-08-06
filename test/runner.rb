# Run more than one test at a time.
#
# usage:
#
#     bin/natalie test/runner.rb spec/core/array/pack/{a,b}_spec.rb
#
# usage with flags:
#
#     bin/natalie -c2 test/runner.rb -- -c2 spec/core/array/pack/{a,b}_spec.rb

require_relative 'support/nat_binary'

flags = []
flags << ARGV.shift while ARGV.first.to_s.start_with?('-')

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
