#!/usr/bin/env ruby
# frozen_string_literal: true

require 'socket'
require 'net/http'
require 'openssl'

# Examples without net/http
TCPSocket.open('natalie-lang.org', 80) do |sock|
  sock.write("GET / HTTP/1.1\nHost: natalie-lang.org\nConnection: close\n\n")
  puts sock.read
end

# NATFIXME: Update this example with the changes made to support net/http
TCPSocket.open('natalie-lang.org', 443) do |sock|
  ssl_sock = OpenSSL::SSL::SSLSocket.new(sock)
  ssl_sock.connect
  ssl_sock.write("GET / HTTP/1.1\nHost: natalie-lang.org\nConnection: close\n\n")
  puts ssl_sock.read
end

# Examples with net/http
puts Net::HTTP.get(URI('http://natalie-lang.org/'))
puts Net::HTTP.get(URI('https://natalie-lang.org/specs'))
