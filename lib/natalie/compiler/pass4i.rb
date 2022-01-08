require_relative './base_pass'
require_relative '../vm/instructions'

module Natalie
  class Compiler
    # Compile for our Interpreter VM
    class Pass4i < BasePass
      def initialize(compiler_context)
        super
        self.default_method = nil
        self.warn_on_default = true
      end

      def process_block(exp)
        _, *body = exp
        body[0..-2].each { |node| process_atom(node) }
        body.empty? ? process(s(:nil)) : process_atom(body.last)
      end

      def process_new(exp)
        _, obj = exp
        case obj
        when :StringObject
          # FIXME: maybe handle length
          _, _obj, (_, str), _length = exp
          VM::PushStringInstruction.new(str)
        else
          raise "I don't yet know how to handle new #{obj.inspect}"
        end
      end

      def process_send(exp)
        _, receiver, (_, message), (_, *args), block = exp
        instructions = args.map { |arg| process(arg) }
        instructions << VM::PushIntInstruction.new(args.size)
        if receiver == :self
          instructions << VM::PushSelfInstruction.new
        else
          raise "I do not know how to handle: #{receiver.inspect}"
        end
        instructions << VM::SendInstruction.new(message)
      end

      def process_var_alloc(exp)
        # TODO
      end

      define_method 'process_Value::integer' do |exp|
        _, int = exp
        VM::PushIntInstruction.new(int)
      end
    end
  end
end
