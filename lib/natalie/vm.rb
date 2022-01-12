require_relative 'vm/main_object'

module Natalie
  class VM
    def initialize(instructions)
      @instructions = Compiler2::InstructionManager.new(instructions)
      @stack = []
      @call_stack = []
      @self = MainObject.new
    end

    attr_accessor :self

    attr_reader :stack

    def ip
      @instructions.ip
    end

    def ip=(ip)
      @instructions.ip = ip
    end

    def run
      @instructions.walk do |instruction|
        result = instruction.execute(self)
        break if result == :halt
      end
    end

    def skip_block_of_instructions(until_instruction: Compiler2::EndInstruction, expected_label: nil)
      @instructions.fetch_block(until_instruction: until_instruction, expected_label: expected_label)
    end

    def push(value)
      @stack.push(value)
    end

    def pop
      raise 'out of stack' if @stack.empty?

      @stack.pop
    end

    def push_call(return_ip:, args:)
      @call_stack.push(return_ip: return_ip, args: args, vars: {})
    end

    def pop_call
      raise 'out of call stack' if @call_stack.empty?

      @call_stack.pop
    end

    def current_args
      raise 'out of call stack' if @call_stack.empty?

      @call_stack.last[:args]
    end

    def vars
      raise 'out of call stack' if @call_stack.empty?

      @call_stack.last[:vars]
    end
  end
end
