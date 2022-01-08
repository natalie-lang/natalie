require_relative './multiple_assignment'

module Natalie
  class Compiler
    class MethodArgs < MultipleAssignment
      def initialize(names, value_name, is_proc: false)
        @is_proc = is_proc
        @names = names
        @value_name = value_name
        build_args
      end

      def arity
        num = required_arg_count
        opt = optional_arg_count
        if required_keyword_arg_count > 0
          num += 1
        elsif optional_keyword_arg_count > 0
          opt += 1
        end
        num = -num - 1 if opt > 0
        num
      end

      def set_args
        s(:block, *@args)
      end

      private

      def required_arg_count
        @names.count { |n| n.is_a?(Symbol) && !n.start_with?('*') } +
          @names.count { |n| n.is_a?(Sexp) && n.sexp_type == :masgn }
      end

      def optional_arg_count
        splat_arg_count + optional_named_arg_count
      end

      def splat_arg_count
        @names.count { |n| n.is_a?(Symbol) && n.start_with?('*') }
      end

      def optional_named_arg_count
        return 0 if @is_proc
        @names.count { |n| n.is_a?(Sexp) && %i[lasgn masgn].include?(n.sexp_type) }
      end

      def required_keyword_arg_count
        @names.count { |n| n.is_a?(Sexp) && n.sexp_type == :kwarg && n.size == 2 }
      end

      def optional_keyword_arg_count
        return 0 if @is_proc
        @names.count { |n| n.is_a?(Sexp) && n.sexp_type == :kwarg && n.size == 3 }
      end

      def build_args
        names = arg_names_as_sexps(@names)
        args_have_default = names.map { |e| %i[iasgn lasgn].include?(e.sexp_type) && e.size == 3 }
        defaults = args_have_default.select { |d| d }
        defaults_on_right = defaults.any? && args_have_default.uniq == [false, true]
        @args =
          masgn_paths(s(:masgn, s(:array, *names))).map do |name, path_details|
            path = path_details[:path]
            if name.is_a?(Sexp)
              has_kwargs = names.any? { |name| name.sexp_type == :kwarg || name.sexp_type == :kwsplat }
              case name.sexp_type
              when :splat
                options =
                  s(
                    :struct,
                    value: @value_name,
                    default_value: 'nullptr',
                    splat: true,
                    has_kwargs: has_kwargs,
                    total_count: names.size,
                    default_count: defaults.size,
                    defaults_on_right: defaults_on_right,
                    offset_from_end: path_details[:offset_from_end],
                  )
                value = s(:arg_value_by_path, :env, options, path.size, *path)
                masgn_set(name.last, value)
              when :kwsplat
                value = s(:kwarg_hash, @value_name)
                masgn_set(name.last, value)
              when :kwarg
                if name[2]
                  default_value = name[2]
                else
                  default_value = 'nullptr'
                end
                value = s(:kwarg_value_by_name, :env, @value_name, s(:s, name[1]), default_value)
                masgn_set(name, value)
              else
                if name.size == 3
                  default_value = name.last
                  name = Sexp.from_array(name[0..1])
                else
                  default_value = 'nullptr'
                end
                total_argument_count = names.reject { |name| name.sexp_type == :kwarg || name.sexp_type == :splat }.size
                options =
                  s(
                    :struct,
                    value: @value_name,
                    default_value: default_value,
                    splat: false,
                    has_kwargs: has_kwargs,
                    total_count: total_argument_count,
                    default_count: defaults.size,
                    defaults_on_right: defaults_on_right,
                    offset_from_end: 0,
                  )
                value = s(:arg_value_by_path, :env, options, path.size, *path)
                masgn_set(name, value)
              end
            else
              raise "unknown masgn type: #{name.inspect} (#{exp.file}\##{exp.line})"
            end
          end
      end

      def arg_names_as_sexps(names)
        names.map do |name|
          case name
          when Symbol
            case name.to_s
            when /^\*\*@(.+)/
              s(:kwsplat, s(:iasgn, name[2..-1].to_sym))
            when /^\*\*(.+)/
              s(:kwsplat, s(:lasgn, name[2..-1].to_sym))
            when '**'
              s(:kwsplat, s(:lasgn, :_))
            when /^\*@(.+)/
              s(:splat, s(:iasgn, name[1..-1].to_sym))
            when /^\*(.+)/
              s(:splat, s(:lasgn, name[1..-1].to_sym))
            when '*'
              s(:splat, s(:lasgn, :_))
            when /^@/
              s(:iasgn, name)
            else
              s(:lasgn, name)
            end
          when Sexp
            case name.sexp_type
            when :lasgn, :kwarg
              name
            when :masgn
              s(:masgn, s(:array, *arg_names_as_sexps(name[1..-1])))
            else
              raise "unknown arg type: #{name.inspect}"
            end
          when nil
            s(:lasgn, :_)
          else
            raise "unknown arg type: #{name.inspect}"
          end
        end
      end

      def masgn_set(exp, value)
        exp = super(exp, value)
        exp[0] = :arg_set if exp.sexp_type == :var_set
        exp
      end
    end
  end
end
