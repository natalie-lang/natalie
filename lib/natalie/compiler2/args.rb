require_relative './multiple_assignment'

module Natalie
  class Compiler2
    class Args < MultipleAssignment
      private

      def transform_arg(arg)
        if arg.is_a?(Sexp)
          case arg.sexp_type
          when :kwarg
            transform_keyword_arg(arg)
          when :lasgn
            transform_optional_arg(arg)
          when :masgn
            transform_destructured_arg(arg)
          else
            raise "I don't yet know how to compile #{arg.inspect}"
          end
        elsif arg.start_with?('*')
          transform_splat_arg(arg)
        else
          transform_required_arg(arg)
        end
      end

      def transform_splat_arg(arg)
        name = arg[1..-1]
        if name.empty?
          :noop
        else
          @instructions << variable_set(name)
          @instructions << VariableGetInstruction.new(name) # TODO: could eliminate this if the *splat is the last arg
        end
        :reverse
      end

      def variable_set(name)
        name = name[1..] if name.start_with?('&')
        raise "bad var name: #{name.inspect}" unless name =~ /^[a-z_][a-z0-9_]*/
        VariableSetInstruction.new(name, local_only: true)
      end
    end
  end
end
