module Natalie
  class Compiler
    # Replace non-captured variable references with local C variables
    class Pass3 < SexpProcessor
      def initialize(compiler_context)
        super()
        self.default_method = :process_sexp
        self.warn_on_default = false
        self.require_empty = false
        self.strict = true
        @compiler_context = compiler_context
        @env = { vars: {} }
      end

      def go(ast)
        process(ast)
      end

      def process_nat_var_get(exp)
        (_, env, var) = exp
        if var[:captured]
          exp.new(:nat_var_get, env, s(:s, var[:name]), var[:index])
        else
          exp.new(:l, "#{@compiler_context[:var_prefix]}#{var[:name]}#{var[:var_num]}")
        end
      end

      def process_nat_var_set(exp)
        (_, env, var, value) = exp
        if var[:captured]
          exp.new(:nat_var_set, env, s(:s, var[:name]), var[:index], process(value))
        else
          exp.new(:c_assign, "#{@compiler_context[:var_prefix]}#{var[:name]}#{var[:var_num]}", process(value))
        end
      end

      def process_sexp(exp)
        (name, *args) = exp
        exp.new(name, *args.map { |a| process_arg(a) })
      end

      private

      def process_arg(exp)
        case exp
        when Sexp
          process(exp)
        when String, Symbol, Integer, nil
          exp
        else
          raise "unknown node type: #{exp.inspect}"
        end
      end
    end
  end
end
