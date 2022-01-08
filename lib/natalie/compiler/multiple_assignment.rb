module Natalie
  class Compiler
    class MultipleAssignment
      def initialize(exp, value_name)
        @exp = exp
        @value_name = value_name
      end

      attr_reader :exp, :value_name

      def generate
        _, names, val = exp
        names = names[1..-1]
        val = val.last if val.sexp_type == :to_ary
        exp.new(:block, s(:declare, value_name, s(:to_ary, :env, val, :false)), *paths(exp, value_name))
      end

      private

      def paths(exp, value_name)
        masgn_paths(exp).map do |name, path_details|
          path = path_details[:path]
          if name.is_a?(Sexp)
            if name.sexp_type == :splat
              if name.size == 1
                # nameless splat
                s(:block)
              else
                options =
                  s(
                    :struct,
                    value: value_name,
                    default_value: s(:nil),
                    splat: true,
                    offset_from_end: path_details[:offset_from_end],
                  )
                value = s(:array_value_by_path, :env, options, path.size, *path)
                masgn_set(name.last, value)
              end
            else
              default_value = name.size == 3 ? name.pop : s(:nil)
              options = s(:struct, value: value_name, default_value: default_value, splat: false, offset_from_end: 0)
              value = s(:array_value_by_path, :env, options, path.size, *path)
              masgn_set(name, value)
            end
          else
            raise "unknown masgn type: #{name.inspect} (#{exp.file}\##{exp.line})"
          end
        end
      end

      # Ruby blows the stack at around this number, so let's limit Natalie as well.
      # Anything over a few dozen is pretty crazy, actually.
      MAX_MASGN_PATH_INDEX = 131_044

      def masgn_paths(exp, prefix = [])
        _, (_, *names) = exp
        splatted = false
        names_without_kwargs = names.reject { |n| n.is_a?(Sexp) && n.sexp_type == :kwarg }
        names
          .each_with_index
          .each_with_object({}) do |(e, index), hash|
            raise 'destructuring assignment is too big' if index > MAX_MASGN_PATH_INDEX
            if e.is_a?(Sexp) && e.sexp_type == :masgn
              hash.merge!(masgn_paths(e, prefix + [index]))
            elsif e.sexp_type == :splat
              splatted = true
              hash[e] = { path: prefix + [index], offset_from_end: names_without_kwargs.size - index - 1 }
            elsif e.sexp_type == :kwsplat
              splatted = true
              hash[e] = { path: prefix + [index], offset_from_end: names.size - index - 1 }
            elsif splatted
              hash[e] = { path: prefix + [(names.size - index) * -1] }
            else
              hash[e] = { path: prefix + [index] }
            end
          end
      end

      def masgn_set(exp, value)
        case exp.sexp_type
        when :cdecl
          exp.new(:const_set, :self, s(:intern, exp.last), value)
        when :gasgn
          exp.new(:global_set, :env, s(:intern, exp.last), value)
        when :iasgn
          exp.new(:ivar_set, :self, :env, s(:intern, exp.last), value)
        when :lasgn, :kwarg
          exp.new(:var_set, :env, s(:s, exp[1]), value)
        when :attrasgn
          _, receiver, message, attr = exp
          args = s(:args, attr, value)
          exp.new(:public_send, receiver, s(:intern, message), args)
        else
          raise "unknown masgn type: #{exp.inspect} (#{exp.file}\##{exp.line})"
        end
      end
    end
  end
end
