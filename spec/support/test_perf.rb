# This file is used to count instructions for a few small programs and send them to natalie-lang.org.

require 'fileutils'
require 'json'
require 'net/http'
require 'uri'

totals = {}
{
  hello:     '/tmp/hello',
  fib:       '/tmp/fib',
  boardslam: '/tmp/boardslam 3 5 1',
  nat:       'bin/nat -c /tmp/bs examples/boardslam.rb',
}.each do |name, command|
  unless name == :nat
    system "bin/natalie -c /tmp/#{name} examples/#{name}.rb"
  end
  out_path = "/tmp/callgrind.#{name}.out"
  total_path = "/tmp/callgrind.#{name}.total"
  system "valgrind --tool=callgrind --callgrind-out-file=#{out_path} #{command}"
  system "callgrind_annotate #{out_path} | grep 'PROGRAM TOTALS' | sed 's/,//g' | awk '{ print $1 }' " \
         "> #{total_path}"
  totals[name] = File.read(total_path).to_i
end
pp totals

unless ENV['STATS_API_SECRET'].to_s.size > 0
  puts 'No STATS_API_SECRET set, aborting.'
  exit 0
end

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
