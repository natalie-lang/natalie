# This file is used to count instructions for a few small programs and send them to natalie-lang.org.

require 'json'
require 'uri'
require 'net/http'

unless ENV['STATS_API_SECRET'].to_s.size > 0
  puts 'No STATS_API_SECRET set, aborting.'
  exit 1
end


totals = {}
{
  hello:     './hello',
  fib:       './fib',
  boardslam: './boardslam 3 5 1',
}.each do |name, command|
  system "bin/natalie -c #{name} examples/#{name}.rb"
  system "valgrind --tool=callgrind --callgrind-out-file=callgrind.#{name}.out #{command}"
  system "callgrind_annotate callgrind.#{name}.out | grep 'PROGRAM TOTALS' | sed 's/,//g' | awk '{ print $1 }' " \
         "> callgrind.#{name}.total"
  totals[name] = File.read("callgrind.#{name}.total").to_i
end
pp totals

stats = {
  commit: ENV.fetch('GIT_SHA'),
  branch: ENV.fetch('GIT_BRANCH'),
  ir: totals
}

uri = URI('https://stats.natalie-lang.org/perf_stats')
https = Net::HTTP.new(uri.host, uri.port)
https.use_ssl = true
form = URI.encode_www_form('secret' => ENV['STATS_API_SECRET'], 'stats' => stats.to_json)
response = https.post(uri.path, form)

if response.code == '201' && response.body.strip == 'ok'
  puts 'Stats accepted by server.'
else
  puts 'Server did not respond as expected. I expected to see a 201 and an "ok" body.'
  p(status: response.code, body: response.body)
  exit 1
end
