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
        _, call, args, *body = exp

        if call[1]&.sexp_type == :array && body.length == 1
          # FIXME: This might unwrap/duplicate big blocks of code, if they are
          #        passed as the function body
          # FIXME: The implementation of these might not honour name shadowing of
          #        nested lambdas/iters, yet
          # FIXME: We do not support re-assignment of any argument passed in yet,
          #        for that we would need a more sophisticated inlining algorithm
          #        => bail for now
          if !(
               args.any? do |arg|
                 body.flatten.any? { |sexp| sexp.is_a?(Sexp) && sexp.sexp_type == :lasgn && sexp[1] == arg }
               end
             )
            _, *values = call[1]
            case call[2]
            when :min
              return canary_array_min(exp, values, args, *body)
            when :max
              return canary_array_max(exp, values, args, *body)
            when :any?
              return canary_array_any(exp, values, args, *body)
            when :all?
              return canary_array_all(exp, values, args, *body)
            end
          end
        end
        return exp.new(:iter, process_call(call), args, *body.map { |e| process_atom(e) })
      end

      def process_call(exp)
        exp = exp.new(*exp.map { |e| process_atom(e) })
        _, receiver, method, *args = exp
        return exp unless receiver && !receiver.empty?

        t, *values = receiver
        return exp unless t == :array

        case method
        when :include?
          return canary_array_include(exp, values, args)
        when :min
          return canary_array_min(exp, values, args)
        when :max
          return canary_array_max(exp, values, args)
        when :any?
          return canary_array_any(exp, values, args)
        when :all?
          return canary_array_all(exp, values, args)
        end
        return exp
      end

      def inline_arguments_into_block(block, args, values)
        # Replaces all mentions of a variable with an value passes into the block
        raise CompilerError.new("ICE: Passed invalid arguments block to inlining") unless args.sexp_type == :args
        args[1..].zip(values) do
          | name, value |
          block = block.map { |expr| ((expr == s(:lvar, name)) ? value : expr) }
        end
        return block
      end

      def canary_array_include(exp, values, args)
        unless (args.length == 1)
          raise ArgumentError.new(
                  "(#{exp.file}:#{exp.line}) Wrong number of arguments for Array#include? (expected 1 given #{args.length.to_s})",
                )
        end
        return exp.new(:false) if values.empty?

        first, *rest = values
        return exp.new(:call, first, :==, args[0]) if values.length == 1
        return exp.new(:or, exp.new(:call, first, :==, args[0]), canary_array_include(exp, rest, args))
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

        # Fast paths
        return exp.new(:nil) if values.empty?
        return values[0] if values.length == 1

        first, second, *rest = values
        if block.nil?
          return(
            exp.new(
              :if,
              exp.new(:call, first, :<, second),
              canary_array_min(exp, [first, *rest]),
              canary_array_min(exp, [second, *rest]),
            )
          )
        end
        evaluated_block = inline_arguments_into_block(block, args, [first, second])
        return(
          exp.new(
            :if,
            exp.new(:call, evaluated_block, :<, s(:lit, 0)),
            canary_array_min(exp, [first, *rest], args, block),
            canary_array_min(exp, [second, *rest], args, block),
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

        # Fast paths
        return exp.new(:nil) if values.empty?
        return values[0] if values.length == 1

        first, second, *rest = values
        if block.nil?
          return(
            exp.new(
              :if,
              exp.new(:call, first, :>, second),
              canary_array_max(exp, [first, *rest]),
              canary_array_max(exp, [second, *rest]),
            )
          )
        end

        evaluated_block = inline_arguments_into_block(block, args, [first, second])

        return(
          exp.new(
            :if,
            exp.new(:call, evaluated_block, :>, 0),
            canary_array_max(exp, [first, *rest], args, block),
            canary_array_max(exp, [second, *rest], args, block),
          )
        )
      end

      def canary_array_any(exp, values, args = nil, block = nil)
        if (block.nil? && !args.nil? && args.length != 0)
          raise ArgumentError.new(
                  "(#{exp.file}:#{exp.line}) Wrong number of arguments for Array#any? (expected 0 given #{args&.length.to_s})",
                )
        end
        if (block && (args.nil? || args.length != 2))
          raise ArgumentError.new(
                  "(#{exp.file}:#{exp.line}) Wrong number of arguments for Array#any? (expected 1 given #{args.nil? ? 0 : (args.length - 1).to_s})",
                )
        end

        return exp.new(:false) if values.empty?

        first, *rest = values
        if block.nil?
          return exp.new(:call, first, :is_truthy) if values.length == 1
          return exp.new(:or, exp.new(:call, first, :is_truthy), canary_array_any(exp, rest))
        end

        inlined_block = inline_arguments_into_block(block, args, [first])
        return exp.new(:call, inlined_block, :is_truthy) if values.length == 1
        return exp.new(:or, exp.new(:call, inlined_block, :is_truthy), canary_array_any(exp, rest, args, block))
      end

      def canary_array_all(exp, values, args = nil, block = nil)
        if (block.nil? && !args.nil? && args.length != 0)
          raise ArgumentError.new(
                  "(#{exp.file}:#{exp.line}) Wrong number of arguments for Array#all? (expected 0 given #{args&.length.to_s})",
                )
        end
        if (block && (args.nil? || args.length != 2))
          raise ArgumentError.new(
                  "(#{exp.file}:#{exp.line}) Wrong number of arguments for Array#all? (expected 1 given #{args.nil? ? 0 : (args.length - 1).to_s})",
                )
        end

        return exp.new(:false) unless values.empty?

        first, *rest = values
        if block.nil?
          return exp.new(:call, first, :is_truthy) if values.length == 1
          return exp.new(:and, exp.new(:call, first, :is_truthy), canary_array_all(exp, rest))
        end

        inlined_block = inline_arguments_into_block(block, args, [first])
        return exp.new(:call, inlined_block, :is_truthy) if values.length == 1
        return exp.new(:and, exp.new(:call, inlined_block, :is_truthy), canary_array_all(exp, rest, args, block))
      end
    end
  end
end
