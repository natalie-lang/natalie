# Run more than one test at a time.
#
# usage:
#
#     bin/natalie test/runner.rb spec/core/array/pack/{a,b}_spec.rb
#

require_relative 'support/nat_binary'

ARGV.each do |path|
  if File.directory?(path)
    $stderr.puts "WARNING: skipping directory #{path}"
    next
  end
  pid = spawn(NAT_BINARY, path)
  Process.wait(pid)
  exit $?.exitstatus unless $?.success?
end
