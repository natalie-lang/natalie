require_relative './base_instruction'

module Natalie
  class Compiler
    class VariableDeclareInstruction < BaseInstruction
      def initialize(name, local_only: false)
        @name = name.to_sym
        @local_only = local_only
      end

      attr_reader :name, :local_only

      attr_accessor :meta

      def to_s
        s = "variable_declare #{@name}"
        s << ' local' if @local_only
        s
      end

      def generate_llvm(transform)
        transform.symbol_name(name.to_s)
        # Environment slots are initialized to nil at function entry.
      end
    end
  end
end
