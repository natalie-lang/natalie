# frozen_string_literal: true

require_relative './base_instruction'

module Natalie
  class Compiler
    class VariableGetInstruction < BaseInstruction
      def initialize(name, default_to_nil: false)
        @name = name.to_sym
        @default_to_nil = default_to_nil
      end

      attr_reader :name, :default_to_nil

      attr_accessor :meta

      def to_s
        s = "variable_get #{@name}"
        s << ' (default_to_nil)' if @default_to_nil
        s
      end

      def generate_llvm(transform)
        if (slot = transform.local_variable_slot(self))
          transform.push_call("load volatile i64, ptr #{slot}")
          return
        end
        depth, index = transform.variable_address(self)
        transform.push_call("call i64 @nat_llvm_var_get(ptr %env, i64 #{depth}, i64 #{index})")
      end
    end
  end
end
