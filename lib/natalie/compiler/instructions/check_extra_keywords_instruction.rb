require_relative './base_instruction'

module Natalie
  class Compiler
    class CheckExtraKeywordsInstruction < BaseInstruction
      def to_s
        "check_extra_keywords"
      end

      def generate(transform)
        hash = transform.peek
        transform.exec("env->ensure_no_extra_keywords(#{hash})")
      end

      def execute(vm)
        hash = vm.peek
        unknown = hash.keys
        if unknown.size == 1
          raise ArgumentError, "unknown keyword: #{unknown.first.inspect}"
        elsif unknown.any?
          raise ArgumentError, "unknown keywords: #{unknown.map(&:inspect).join ', '}"
        end
      end

      def serialize
        [instruction_number].pack('C')
      end

      def self.deserialize(_)
        new
      end
    end
  end
end
