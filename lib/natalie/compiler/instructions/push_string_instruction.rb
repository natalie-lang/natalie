require_relative './base_instruction'

module Natalie
  class Compiler
    class PushStringInstruction < BaseInstruction
      def initialize(string, bytesize: string.bytesize, encoding: Encoding::UTF_8, status: nil)
        super()
        @string = string
        @bytesize = bytesize
        @encoding = encoding
        @status = status
      end

      def frozen=(value)
        if value
          @status = :frozen
        elsif @status == :frozen
          @status = nil
        else
          raise ArgumentError, 'unable to determine desired value of status'
        end
      end

      def to_s
        "push_string #{@string.inspect}, #{@bytesize}, #{@encoding.name}#{frozen? ? ', frozen' : ''}#{chilled? ? ', chilled' : ''}"
      end

      def generate_llvm(transform)
        status = { nil => 0, :chilled => 1, :frozen => 2 }.fetch(@status)
        cache = frozen? ? transform.literal_cache([@string.b, @encoding.name]) : 'null'
        transform.push_call(
          "call i64 @nat_llvm_string(ptr %env, ptr #{transform.string(@string)}, i64 #{@bytesize}, " \
            "ptr #{transform.string(@encoding.name)}, i32 #{status}, ptr #{cache})",
        )
      end

      private

      def frozen?
        @status == :frozen
      end

      def chilled?
        @status == :chilled
      end
    end
  end
end
