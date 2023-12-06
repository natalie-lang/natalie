#!/usr/bin/env ruby

require 'uri'
require 'open-uri'

TABLES = {
  'JIS0208' => 'https://encoding.spec.whatwg.org/index-jis0208.txt',
  'JIS0212' => 'https://encoding.spec.whatwg.org/index-jis0212.txt',
}.freeze

TABLES.each do |table_name, url|
  lines = URI.open(url).readlines(chomp: true) # rubocop:disable Security/Open
  index = lines.grep_v(/^#|^\s*$/).map(&:split).each_with_object({}) do |(idx, value, *), hash|
    hash[idx.to_i] = value
  end

  print "const long #{table_name}[] = {"

  0.upto(index.keys.max).each do |i|
    value = index[i]

    puts if i % 10 == 0

    if value
      print '0x%X' % value
    else
      print '-1'
    end

    print ', ' unless i == index.keys.max
  end

  puts '};'
  puts "long #{table_name}_max = #{index.keys.max};"
  puts
end
