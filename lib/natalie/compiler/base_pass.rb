module Natalie
  class Compiler
    class BasePass < SexpProcessor
      def initialize(compiler_context)
        super()
        self.default_method = :process_sexp
        self.warn_on_default = false
        self.require_empty = false
        self.strict = true
        @compiler_context = compiler_context
      end

      def go(ast)
        process(ast)
      end

      def process_sexp(exp)
        (name, *args) = exp
        exp.new(name, *args.map { |a| process_atom(a) })
      end

      private

      def temp(name)
        name = name.to_s.gsub(/[^a-zA-Z_]/, '')
        n = @compiler_context[:var_num] += 1
        "#{@compiler_context[:var_prefix]}#{name}#{n}"
      end

      def process_atom(exp)
        case exp
        when Sexp
          process(exp)
        when String, Symbol, Integer, Float, nil, true, false
          exp
        else
          raise "unknown node type: #{exp.inspect}"
        end
      end
    end
  end
end
