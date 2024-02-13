# frozen_string_literal: true
# Timeout long-running blocks
#
# == Synopsis
#
#   require 'timeout'
#   status = Timeout::timeout(5) {
#     # Something that should be interrupted if it takes more than 5 seconds...
#   }
#
# == Description
#
# Timeout provides a way to auto-terminate a potentially long-running
# operation if it hasn't finished in a fixed amount of time.
#
# Previous versions didn't use a module for namespacing, however
# #timeout is provided for backwards compatibility.  You
# should prefer Timeout.timeout instead.
#
# == Copyright
#
# Copyright:: (C) 2000  Network Applied Communication Laboratory, Inc.
# Copyright:: (C) 2000  Information-technology Promotion Agency, Japan

module Timeout
  # The version
  VERSION = "0.4.1"

  # Internal error raised to when a timeout is triggered.
  class ExitException < Exception
    def exception(*) # :nodoc:
      self
    end
  end

  # Raised by Timeout.timeout when the block times out.
  class Error < RuntimeError
    def self.handle_timeout(message) # :nodoc:
      exc = ExitException.new(message)

      begin
        yield exc
      rescue ExitException => e
        raise new(message) if exc.equal?(e)
        raise
      end
    end
  end

  def timeout(sec, klass = nil, message = nil, &block)
    # NATFIXME: For now just run the block without a timeout
    yield(sec)
  end
  module_function :timeout
end
