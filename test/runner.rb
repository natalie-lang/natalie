# Run more than one test at a time.
#
# usage:
#
#     bin/natalie test/runner.rb spec/core/array/pack/{a,b}_spec.rb
#
ARGV.each do |path|
  if File.directory?(path)
    $stderr.puts "WARNING: skipping directory #{path}"
    next
  end
  pid = spawn('bin/natalie', path)
  Process.wait(pid)
  unless $?.success?
    exit $?.exitstatus
  end
end
