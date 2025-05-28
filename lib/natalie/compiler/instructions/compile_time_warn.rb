# frozen_string_literal: true

require_relative 'base_instruction'
require_relative '../string_to_cpp'

module Natalie
  class Compiler
    class CompileTimeWarn < BaseInstruction
      include StringToCpp
      attr_reader :message

      def initialize(message, level:)
        @message = message
        @level = level
      end

      def to_s
        level_str = @level && @level != :default ? " (level=#{@level})" : ''
        "compile_time_warn: #{message}#{level_str}"
      end

      def generate(transform)
        name = transform.temp(:compile_time_warn)
        transform.exec "static const bool #{name} = [&]() {"
        if @level && @level == :verbose
          transform.exec '    if (!GlobalEnv::the()->is_verbose()) {'
          transform.exec '      return false'
          transform.exec '    }'
        end
        transform.exec "    self.send(env, \"warn\"_s, { StringObject::create(#{string_to_cpp(message)}) })"
        transform.exec '    return true'
        transform.exec '}()'
      end

      def execute(_vm)
        unless self.class.seen.key?(self)
          warn(message) if @level.nil? || @level != :verbose || $VERBOSE
          self.class.seen[self] = self
        end
      end

      def self.seen
        @seen ||= {}
      end

      def serialize(_rodata)
        # This needs some restructuring to fit in the bytecode VM. Just skip for now
        ''.b
      end
    end
  end
end
