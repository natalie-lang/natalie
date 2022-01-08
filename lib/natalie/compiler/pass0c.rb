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
        @in_defined_statement = false
      end

      def process_iter(exp)
        call_type, call, args, *body = exp
        return exp.new(:iter, process_call(call, block_fn = [args, body]), args, *body.map { |e| process_atom(e) })
      end

      def process_defined(exp, block_with_args = nil)
        # Note: This might fail on nested defined?s, but who's gonna do that
        @in_defined_statement = true
        exp = exp.new(*exp.map { |e| process_atom(e) })
        @in_defined_statement = false
        return exp
      end

      def type_is_always_truthy(arg)
        # FIXME: Are there any other types that are always truthy?
        return %i[true str lit array].include?(arg)
      end
      def type_is_always_falsey(arg)
        return %i[false nil].include?(arg)
      end

      def process_if(exp, block_with_args = nil)
        exp = exp.new(*exp.map { |e| process_atom(e) })
        if exp.flatten.include?(:lasgn)
          # This is required for now because
          # ```rb
          # x = 1 unless true # s(:if, s(:true), nil, s(:lasgn, :x, s(:lit, 1)))
          # ```
          # gets converted to an if-statement, that does not allocate x in the
          # taken branch, which makes x undefined
          return exp
        end
        _, (boolean, *), true_case, false_case = exp
        return true_case if type_is_always_truthy(boolean)
        return false_case if type_is_always_falsey(boolean)

        return exp
      end

      def process_or(exp, block_with_args = nil)
        exp = exp.new(*exp.map { |e| process_atom(e) })
        _, (t1, *arg1), (t2, *arg2) = exp
        return exp.new(t2, *arg2) if type_is_always_falsey(t1)
        return exp.new(t1, *arg1) if type_is_always_truthy(t1)

        return exp
      end

      def process_and(exp, block_with_args = nil)
        exp = exp.new(*exp.map { |e| process_atom(e) })
        _, (t1, *arg1), (t2, *arg2) = exp
        return s(t1, *arg1) if type_is_always_falsey(t1)
        return exp if !type_is_always_truthy(t1) # t1 is not const-evaluable

        # t2 will be returned even if it is falsey, so we don't have to check it
        return exp.new(t2, *arg2)
      end

      def process_call(exp, block_with_args = nil)
        exp = exp.new(*exp.map { |e| process_atom(e) })
        _, receiver, method, *args = exp
        if receiver && !receiver.empty?
          t, *values = receiver
          if t == :array
            if method == :min && args.empty? && !block_with_args
              return process(canary_array_min(exp, values))
            elsif method == :max && args.empty? && !block_with_args
              return process(canary_array_max(exp, values))
            end
          elsif t == :lit && args.length == 1 && args[0][0] == :lit && !@in_defined_statement
            # we cannot collapse these in defined? statements, because it changes the type
            # from method to expression
            class_arg1 = values[0].class
            class_arg2 = args[0][1].class
            if [Integer, Float].include?(class_arg1) && [Integer, Float].include?(class_arg2)
              case method
              when :+, :-, :*, :<=>
                return canary_const_op(exp, values)
              when :/, :%
                # FIXME: We could propagate NaNs and INFINITIES in the float-division case
                return canary_const_op(exp, values) unless args[0][1] == 0
                # FIXME: This one makes String#* timeout
                # when :**
                #   return canary_const_op(exp, values) unless values[0] < 0
              when :&, :|, :^, :<<, :>>
                return canary_const_op(exp, values) if [class_arg1, class_arg2].all? { |x| x == Integer }
              when :==, :!=, :>, :>=, :<, :<=, :===
                return canary_const_bool_op(exp, values)
              end
            end
          elsif args.length == 0
            if method == :!
              # This can fail and fallback, so we should not try to process it again
              return canary_const_not(exp, values)
            end
          end
        end
        return exp
      end

      def canary_array_min(exp, values)
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

      def canary_array_max(exp, values)
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

      def canary_const_op(exp, values)
        _, (_, arg1), op, (_, arg2) = exp
        return exp.new(:lit, arg1.send(op, arg2))
      end
      def canary_const_bool_op(exp, values)
        _, (_, arg1), op, (_, arg2) = exp
        return exp.new(:true) if arg1.send(op, arg2)
        return exp.new(:false)
      end

      def canary_const_not(exp, values)
        _, (type, *), _not = exp
        return exp.new(:false) if type_is_always_truthy(type)
        return exp.new(:true) if type_is_always_falsey(type)
        return exp
      end
    end
  end
end
