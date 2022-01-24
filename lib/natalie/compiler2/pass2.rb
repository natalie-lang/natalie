require_relative './base_pass'

module Natalie
  class Compiler2
    class Pass2 < BasePass
      # Variable Allocation Pass
      def initialize(instructions)
        @instructions = instructions
        @context_stack = []
        @lvars = {}
        @ip = 0
      end

      def enter_context
        @context_stack << @lvars
        @lvars = {}
      end

      def exit_context
        old_lvars = @lvars
        @lvars = @context_stack.pop
        old_lvars
      end

      def transform_variable_set(inst)
        @lvars[inst.name] = inst.name
      end

      def transform_define_method(inst)
        ip = @ip
        all_instructions = @instructions
        instruction_manager = InstructionManager.new(all_instructions[ip + 1..])
        
        @instructions = instruction_manager.fetch_block()
        old_block_size = @instructions.size

        enter_context
        transform
        exit_context

        new_instructions = []
        new_instructions << all_instructions[..ip]
        new_instructions << @instructions
        new_instructions << all_instructions[ip + old_block_size + 1..]

        @ip = ip + @instructions.size - 1
        @instructions = new_instructions.flatten()
      end

      alias define_block transform_define_method
      alias define_class transform_define_method


      def transform
        while @ip < @instructions.size
          instruction = @instructions[@ip]
          instruction_handler = "transform_#{instruction.to_s.split[0]}"
          if respond_to?(instruction_handler)
            send(instruction_handler, instruction)
          end
          @ip += 1
        end

        @instructions.prepend(*@lvars.map { |_, var_name| DeclareVarInstruction.new(var_name) })
      end
    end
  end
end
