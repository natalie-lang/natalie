#!/usr/bin/env ruby

# usage:
#
#     bin/natalie path/to/some/spec -f yaml | spec/support/natfixme.rb

require 'yaml'

exceptions = YAML.load($stdin.read)['exceptions']

exceptions.each do |exception|
  relevant_backtrace_line = exception['backtrace'].detect { |l| l =~ /^spec/ && l !~ /^spec\/support/ }
  puts "NATFIXME 'some description', exception: #{exception['class']}, message: #{exception['message'].inspect} do"
  puts relevant_backtrace_line.split(':')[0..1].join(':')
  puts
end

puts "#{exceptions.size} exception(s)"
