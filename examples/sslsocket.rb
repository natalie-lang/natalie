#!/usr/bin/env ruby
# frozen_string_literal: true

require 'socket'
require 'openssl'

# Warning: OpenSSL support is a work in progress and should not be used for anything other than testing.
TCPSocket.open('natalie-lang.org', 443) do |sock|
  ssl_context = OpenSSL::SSL::SSLContext.new
  ssl_context.set_params({ min_version: :TLS1_3, max_version: :TLS1_3, security_level: 2 })
  ssl_context.session_cache_mode = OpenSSL::SSL::SSLContext::SESSION_CACHE_CLIENT | OpenSSL::SSL::SSLContext::SESSION_CACHE_NO_INTERNAL_STORE

  ssl_sock = OpenSSL::SSL::SSLSocket.new(sock, ssl_context)
  ssl_sock.hostname = 'natalie-lang.org'
  ssl_sock.connect

  ssl_sock.write("GET / HTTP/1.1\r\nHost: natalie-lang.org\r\nConnection: close\r\n\r\n")
  body = ssl_sock.read
  puts body.sub(/\A.*?(<html)/m, '\\1')
end
