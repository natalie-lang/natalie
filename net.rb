#!/usr/bin/env ruby
# frozen_string_literal: true

require 'net/http'

puts Net::HTTP.get(URI('http://natalie-lang.org/'))
puts Net::HTTP.get(URI('https://natalie-lang.org/specs'))
