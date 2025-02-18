# frozen_string_literal: true

require_relative 'base_instruction'
require_relative '../string_to_cpp'

module Natalie
  class Compiler
    class CompileTimeWarn < BaseInstruction
      include StringToCpp
      attr_reader :message

      def initialize(message)
        @message = message
      end

      def to_s
        "compile_time_warn: #{message}"
      end

      def generate(transform)
        name = transform.temp(:compile_time_warn)
        transform.exec "static const bool #{name} = [&]() {"
        transform.exec "    self.send(env, \"warn\"_s, { new StringObject { #{string_to_cpp(message)} } })"
        transform.exec "    return true"
        transform.exec "}()"
      end

      def execute(_vm)
        unless self.class.seen.key?(self)
          warn(message)
          self.class.seen[self] = self
        end
      end

      def self.seen
        @seen ||= {}
      end
    end
  end
end
