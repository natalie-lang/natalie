require_relative './base_pass'

module Natalie
  class Compiler
    # Number every method call with an incrementing call site index.
    class Pass3c < BasePass
      def process_send(exp)
        (type, receiver, method, args, block) = exp
        num = @compiler_context[:call_site_num]
        @compiler_context[:call_site_num] += 1
        exp.new(type,
                process_atom(receiver),
                process_atom(method),
                process_atom(args),
                process_atom(block),
                s(:call_site, num))
      end

      def process_public_send(exp)
        process_send(exp)
      end
    end
  end
end
