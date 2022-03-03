# Run more than one test at a time.
#
# usage:
#
#     bin/natalie test/runner.rb spec/core/array/pack/{a,b}_spec.rb
#

require 'natalie/macros'

macro!(:require_argv) do
  requires = []
  ARGV.each do |path|
    if File.directory?(path)
      warn "WARNING: skipping directory #{path}"
      next
    end
    requires << s(:call, nil, :require, s(:str, path))
  end
  s(:block, *requires)
end

require_argv
