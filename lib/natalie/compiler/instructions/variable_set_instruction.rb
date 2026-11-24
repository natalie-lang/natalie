# frozen_string_literal: true

require_relative './base_instruction'

module Natalie
  class Compiler
    class VariableSetInstruction < BaseInstruction
      def initialize(name, local_only: false)
        @name = name.to_sym
        @local_only = local_only
      end

      attr_reader :name, :local_only

      attr_accessor :meta

      def to_s
        s = "variable_set #{@name}"
        s << ' local' if @local_only
        s
      end

      def generate_llvm(transform)
        if (slot = transform.local_variable_slot(self))
          transform.emit "store volatile i64 #{transform.pop}, ptr #{slot}"
          return
        end
        depth, index = transform.variable_address(self)
        transform.emit "call void @nat_llvm_var_set(ptr %env, i64 #{depth}, i64 #{index}, i64 #{transform.pop})"
      end
    end
  end
end
