require_relative '../../../compiler/base_pass'

module Natalie
  class Compiler2
    # A 'canary' pass that tries to simplify well-known ruby pattern which would
    # normally cost allocating a data-structure
    # https://ruby-compilers.com/examples/#example-program-canaryrb
    class CanaryArrayFunctions < ::Natalie::Compiler::BasePass
      def initialize(context)
        super
        self.default_method = nil
        self.warn_on_default = true
        self.require_empty = false
        self.strict = false
      end

      def process_iter(exp)
        call_type, call, args, *body = exp
        return exp.new(:iter, process_call(call), args, *body.map { |e| process_atom(e) })
      end

      def process_call(exp)
        exp = exp.new(*exp.map { |e| process_atom(e) })
        _, receiver, method, *args = exp
        return exp unless receiver && !receiver.empty?

        t, *values = receiver
        return exp unless t == :array

        case method
        when :min
          return canary_array_min(exp, values, args)
        when :max
          return canary_array_max(exp, values, args)
        end
        return exp
      end

      def canary_array_min(exp, values, args = nil, block = nil)
        if (block.nil? && !args.nil? && args.length != 0)
          raise ArgumentError.new(
                  "(#{exp.file}:#{exp.line}) Wrong number of arguments for Array#min (expected 0, given #{args&.length.to_s})",
                )
        end
        if (block && (args.nil? || args.length != 3))
          raise ArgumentError.new(
                  "(#{exp.file}:#{exp.line}) Wrong number of arguments for Array#min (expected 2, given #{args.nil? ? 0 : (args.length - 1).to_s})",
                )
        end

        return exp.new(:nil) if values.empty?
        return values[0] if values.length == 1
        first, second, *rest = values
        return(
          exp.new(
            :if,
            exp.new(:call, first, :<, second),
            canary_array_min(exp, [first, *rest]),
            canary_array_min(exp, [second, *rest]),
          )
        )
      end

      def canary_array_max(exp, values, args = nil, block = nil)
        if (block.nil? && !args.nil? && args.length != 0)
          raise ArgumentError.new(
                  "(#{exp.file}:#{exp.line}) Wrong number of arguments for Array#max (expected 0 given #{args&.length.to_s})",
                )
        end
        if (block && (args.nil? || args.length != 3))
          raise ArgumentError.new(
                  "(#{exp.file}:#{exp.line}) Wrong number of arguments for Array#max (expected 2 given #{args.nil? ? 0 : (args.length - 1).to_s})",
                )
        end

        return exp.new(:nil) if values.empty?
        return values[0] if values.length == 1
        first, second, *rest = values
        return(
          exp.new(
            :if,
            exp.new(:call, first, :>, second),
            canary_array_max(exp, [first, *rest]),
            canary_array_max(exp, [second, *rest]),
          )
        )
      end
    end
  end
end
