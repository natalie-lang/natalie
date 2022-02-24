require_relative './base_instruction'

module Natalie
  class Compiler2
    class CreateLambdaInstruction < BaseInstruction
      def to_s
        'create_lambda'
      end

      def generate(transform)
        block = transform.pop
        transform.exec_and_push(:lambda, "Value(new ProcObject(#{block}, ProcObject::ProcType::Lambda))")
      end

      def execute(vm)
        block = vm.pop
        vm.push(convert_to_lambda(&block))
      end

      private

      # https://stackoverflow.com/a/2946734
      def convert_to_lambda(&block)
        obj = Object.new
        obj.define_singleton_method(:_, &block)
        return obj.method(:_).to_proc
      end
    end
  end
end
