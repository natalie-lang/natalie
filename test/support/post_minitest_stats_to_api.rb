require 'json'
require 'uri'
require 'net/http'

stats = ARGF.read.split(', ').map(&:split).each_with_object({}) { |(c, l), h| h[l] = c.to_i }
uri = URI('https://stats.natalie-lang.org/self_hosted_stats')
https = Net::HTTP.new(uri.host, uri.port)
https.use_ssl = true
puts https.post(uri.path, URI.encode_www_form('stats' => stats.to_json, 'secret' => ENV['STATS_API_SECRET']))
