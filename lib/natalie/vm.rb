require_relative 'vm/main_object'

module Natalie
  class VM
    def initialize(instructions, path:)
      @instructions = instructions
      @stack = []
      @call_stack = [{ scope: { vars: {} } }]
      @self = build_main
      @method_visibility = :public
      @path = path
      @global_variables = {
        "$0": @path,
        "$stderr": $stderr,
        "$stdout": $stdout,
      }
    end

    attr_accessor :self, :method_visibility, :global_variables, :rescued

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
        if %i[break_out halt next return].include?(result)
          return result
        end
      end
      result
    end

    def run_next_instruction
      instruction = @instructions.next
      instruction.execute(self)
    end

    def skip_block_of_instructions(until_instruction: Compiler2::EndInstruction, expected_label: nil)
      @instructions.fetch_block(until_instruction: until_instruction, expected_label: expected_label)
    end

    def push(value)
      @stack.push(value)
    end

    def peek
      raise 'out of stack' if @stack.empty?

      @stack.last
    end

    alias result peek

    def pop
      raise 'out of stack' if @stack.empty?

      @stack.pop
    end

    def push_call(name:, return_ip:, args:, scope:, block:)
      @call_stack.push(name: name, return_ip: return_ip, args: args, scope: scope, block: block)
    end

    def pop_call
      raise 'out of call stack' if @call_stack.empty?

      @call_stack.pop
    end

    def args
      raise 'out of call stack' if @call_stack.empty?

      @call_stack.last[:args]
    end

    def scope
      raise 'out of call stack' if @call_stack.empty?

      @call_stack.last[:scope]
    end

    def block
      raise 'out of call stack' if @call_stack.empty?

      @call_stack.last[:block]
    end

    def method_name
      raise 'out of call stack' if @call_stack.empty?

      @call_stack.last[:name]
    end

    def find_var(name, local_only: false)
      s = scope
      loop do
        if s[:vars].key?(name)
          return s.dig(:vars, name)
        end
        if s[:parent] && !local_only
          s = s[:parent]
        else
          break
        end
      end
    end

    def with_self(obj)
      self_was = @self
      @self = obj
      yield
    ensure
      @self = self_was
    end

    def self.compile_and_run(ast, path:)
      compiler = Compiler2.new(ast, path, interpret: true)
      VM.new(compiler.instructions, path: path).run
    end

    private

    def build_main
      main = Object.new
      def main.inspect; 'main'; end
      def main.define_method(name, &block); Object.define_method(name, &block); end
      def main.private(*args); Object.send(:private, *args); end
      def main.protected(*args); Object.send(:potected, *args); end
      def main.public(*args); Object.send(:public, *args); end
      main
    end
  end
end
