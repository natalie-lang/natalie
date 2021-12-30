require_relative './base_pass'

module Natalie
  class Compiler
    # A 'canary' pass that tries to simplify well-known ruby pattern which would 
    # normally cost allocating a data-structure
    # https://ruby-compilers.com/examples/#example-program-canaryrb
    class Pass0c < BasePass
      def initialize(context)
        super
        self.default_method = nil
        self.warn_on_default = true
        self.require_empty = false
        self.strict = false
      end

      def process_iter(exp)
        (call_type, call, args, *body) = exp
        return exp.s(
          :iter,
          process_call(call, block_fn=[args,body]),
          args,
          *body.map {|inst| inst.is_a?(Sexp) ? process(inst) : inst}
        )
      end

      def process_call(exp, block_with_args=nil)
        (_, receiver, method, *args) = exp
        if receiver && !receiver.empty?
          t, *values = receiver
          if t == :array
            if method == :min && args.empty? && !block_with_args
              return process(canary_array_min(exp, values))
            elsif method == :max && args.empty? && !block_with_args
              return process(canary_array_max(exp, values))
            end
          end
        end
        exp.s(*exp.map {|sub_exp| sub_exp.is_a?(Sexp) ? process(sub_exp) : sub_exp})
      end

      def canary_array_min(exp, values)
        return exp.s(:nil) if values.empty?
        return values[0] if values.length == 1
        first, second, *rest = values
        return exp.s(:if, exp.s(:call, first, :<, second), canary_array_min(exp, [first, *rest]), canary_array_min(exp, [second, *rest]))
      end

      def canary_array_max(exp, values)
        return exp.s(:nil) if values.empty?
        return values[0] if values.length == 1
        first, second, *rest = values
        return exp.s(:if, exp.s(:call, first, :>, second), canary_array_max(exp, [first, *rest]), canary_array_max(exp, [second, *rest]))
      end

    end
  end
end
