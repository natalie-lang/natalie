module Natalie
  class Compiler2
    class Arity
      def initialize(args, is_proc:)
        if args == 0
          @args = []
        elsif args.is_a?(Sexp) && args.sexp_type == :args
          @args = args[1..]
          @args.pop if @args.last.is_a?(Symbol) && @args.last.start_with?('&')
        else
          raise "expected args sexp, but got: #{args.inspect}"
        end
        @is_proc = is_proc
      end

      attr_reader :args

      def arity
        num = required_count
        opt = optional_count
        if required_keyword_count > 0
          num += 1
        elsif optional_keyword_count > 0
          opt += 1
        end
        num = -num - 1 if opt > 0
        num
      end

      private

      def splat_count
        args.count do |arg|
          arg.is_a?(Symbol) && arg.start_with?('*')
        end
      end

      def required_count
        args.count do |arg|
          (arg.is_a?(Symbol) && !arg.start_with?('*')) ||
            (arg.is_a?(Sexp) && arg.sexp_type == :masgn)
        end
      end

      def optional_count
        splat_count + optional_named_count
      end

      def optional_named_count
        return 0 if @is_proc
        args.count do |arg|
          arg.is_a?(Sexp) && %i[lasgn masgn].include?(arg.sexp_type)
        end
      end

      def required_keyword_count
        args.count do |arg|
          arg.is_a?(Sexp) && arg.sexp_type == :kwarg && arg.size == 2
        end
      end

      def optional_keyword_count
        return 0 if @is_proc
        args.count do |arg|
          arg.is_a?(Sexp) && arg.sexp_type == :kwarg && arg.size == 3
        end
      end
    end
  end
end
