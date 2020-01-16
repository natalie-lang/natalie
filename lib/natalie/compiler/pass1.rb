module Natalie
  class Compiler
    # Rewrite S-expressions from Ruby to C
    class Pass1 < SexpProcessor
      def initialize
        super
        @var_num = 0
      end

      attr_accessor :var_num, :var_prefix

      def rewrite_alias(exp)
        (_, (_, new_name), (_, old_name)) = exp
        exp.new(:block,
          s(:nat_alias, :env, :self, s(:s, new_name), s(:s, old_name)),
          s(:nil))
      end

      def rewrite_and(exp)
        (_, lhs, rhs) = exp
        exp.new(:c_if, s(:nat_truthy, lhs), rhs, lhs)
      end

      def rewrite_array(exp)
        return exp if %i[resbody splat masgn].include?(context.first)
        (_, *items) = exp
        arr = temp('arr')
        items = items.map do |item|
          if item.sexp_type == :splat
            s(:nat_array_push_splat, :env, arr, item.last)
          else
            s(:nat_array_push, arr, rewrite(item))
          end
        end
        exp.new(:block,
          s(:declare, arr, s(:nat_array, :env)),
          *items.compact,
          arr)
      end

      def rewrite_attrasgn(exp)
        (_, receiver, method, *args) = exp
        if args.any? { |a| a.sexp_type == :splat }
          args = s(:args_array, rewrite(s(:array, *args)))
        else
          args = s(:args, *args)
        end
        exp.new(:nat_lookup_or_send, receiver, method, args)
      end

      def rewrite_call(exp)
        return exp if context.first == :defined
        (_, receiver, method, *args) = exp
        (_, block_pass) = args.pop if args.last&.sexp_type == :block_pass
        if args.any? { |a| a.sexp_type == :splat }
          args = s(:args_array, rewrite(s(:array, *args)))
        else
          args = s(:args, *args)
        end
        proc_name = temp('proc_to_block')
        sexp_type = receiver ? :nat_send : :nat_lookup_or_send
        if block_pass
          exp.new(:block,
            s(:declare, proc_name, block_pass),
            s(sexp_type, receiver || :self, method, args, "#{proc_name}->block"))
        else
          exp.new(sexp_type, receiver || :self, method, args, 'NULL')
        end
      end

      def rewrite_cdecl(exp)
        (_, name, value) = exp
        exp.new(:env_set, :env, s(:s, name), rewrite(value))
      end

      def rewrite_class(exp)
        (_, name, superclass, *body) = exp
        superclass ||= s(:const, 'Object')
        fn = temp('class_body')
        klass = temp('class')
        exp.new(:block,
          s(:fn2, fn, rewrite(s(:block, *body))),
          s(:declare, klass, s(:env_get, :env, s(:s, name))),
          s(:c_if, s(:not, klass),
            s(:block,
              s(:set, klass, s(:nat_subclass, :env, rewrite(superclass), s(:s, name))),
              s(:env_set, :env, s(:s, name), klass))),
          s(:nat_call, fn, "#{klass}->env", klass))
      end

      def rewrite_colon2(exp)
        (_, parent, name) = exp
        parent_name = temp('parent')
        exp.new(:block,
          s(:declare, parent_name, rewrite(parent)),
          s(:env_get, "#{parent_name}->env", s(:s, name)))
      end

      def rewrite_const(exp)
        return exp if context.first == :defined
        (_, name) = exp
        exp.new(:nat_lookup, :env, s(:s, name))
      end

      def rewrite_defn_internal(exp)
        (_, name, (_, *args), *body) = exp
        name = name.to_s
        fn_name = temp('fn')
        if args.last&.to_s&.start_with?('&')
          block_arg = exp.new(:env_set, :env, s(:s, args.pop.to_s[1..-1]), s(:nat_proc, :env, 'block'))
        end
        assign_args = if args_use_simple_mode?(args)
                        rewrite(s(:masgn_in_args_simple, *args))
                      else
                        s(:nat_multi_assign_args, :env, :self, rewrite(s(:masgn_in_args_full, *args)), :argc, :args)
                      end
        exp.new(:fn, fn_name,
          s(:block,
            s(:env_set, :env, s(:s, '__method__'), s(:nat_string, :env, s(:s, name))),
            s(:env_set_method_name, name),
            assign_args,
            block_arg || s(:block),
            rewrite(s(:block, *body))))
      end

      def rewrite_defn(exp)
        (_, name, args, *body) = exp
        fn = rewrite_defn_internal(exp)
        exp.new(:block,
          fn,
          s(:nat_define_method, :self, s(:s, name), fn[1]),
          s(:nat_symbol, :env, s(:s, name)))
      end

      def rewrite_defs(exp)
        (_, owner, name, args, *body) = exp
        fn = rewrite_defn_internal(exp.new(:defs, name, args, *body))
        exp.new(:block,
          fn,
          s(:nat_define_singleton_method, :env, owner, s(:s, name), fn[1]),
          s(:nat_symbol, :env, s(:s, name)))
      end

      def rewrite_dstr(exp)
        (_, start, *rest) = exp
        string = temp('string')
        segments = rest.map do |segment|
          case segment.sexp_type
          when :evstr
            s(:nat_string_append_nat_string, string, rewrite(s(:call, segment.last, :to_s)))
          when :nat_string
            s(:nat_string_append, string, segment.last)
          else
            raise "unknown dstr segment: #{segment.inspect}"
          end
        end
        exp.new(:block,
          s(:declare, string, s(:nat_string, :env, s(:s, start))),
          *segments,
          string)
      end

      def rewrite_gasgn(exp)
        (_, name, value) = exp
        exp.new(:global_set, :env, s(:s, name), rewrite(value))
      end

      def rewrite_gvar(exp)
        return exp if context.first == :defined
        (_, name) = exp
        exp.new(:global_get, :env, s(:s, name))
      end

      def rewrite_hash(exp)
        (_, *pairs) = exp
        hash = temp('hash')
        inserts = pairs.each_slice(2).map do |(key, val)|
          s(:nat_hash_put, :env, hash, key, val)
        end
        exp.new(:block,
          s(:declare, hash, s(:nat_hash, :env)),
          s(:block, *inserts),
          hash)
      end

      def rewrite_if(exp)
        (_, condition, true_body, false_body) = exp
        true_fn = temp('if_result_true')
        false_fn = true_fn.sub(/true/, 'false')
        exp.new(:block,
          s(:fn2, true_fn, rewrite(true_body || s(:nil))),
          s(:fn2, false_fn, rewrite(false_body || s(:nil))),
          s(:tern, rewrite(condition), true_fn, false_fn))
      end

      def rewrite_iasgn(exp)
        (_, name, value) = exp
        exp.new(:ivar_set, :env, :self, s(:s, name), rewrite(value))
      end

      def rewrite_iter(exp)
        (_, call, (_, *args), *body) = exp
        block_fn = temp('block_fn')
        block = block_fn.sub(/_fn/, '')
        call[call.size-1] = block
        assign_args = if args_use_simple_mode?(args)
                        rewrite(s(:masgn_in_args_simple, *args))
                      else
                        s(:nat_multi_assign_args, :env, :self, rewrite(s(:masgn_in_args_full, *args)), :argc, :args)
                      end
        exp.new(:block,
          s(:fn, block_fn,
            s(:block,
              s(:env_set_method_name, '<block>'),
              assign_args,
              rewrite(s(:block, *body)))),
          s(:declare_block, block, s(:nat_block, :env, :self, block_fn)),
          call)
      end

      def rewrite_ivar(exp)
        return exp if context.first == :defined
        (_, name) = exp
        exp.new(:ivar_get, :env, :self, s(:s, name))
      end

      def rewrite_lambda(exp)
        exp.new(:nat_lambda, :env, 'NULL') # note: the block gets overwritten by rewrite_iter later
      end

      def rewrite_lasgn(exp)
        return exp if context[0..1] == [:array, :masgn]
        return exp if %i[args block_args].include?(context.first)
        (_, name, val) = exp
        exp.new(:env_set, :env, s(:s, name), rewrite(val))
      end

      def rewrite_lit(exp)
        return exp if context.first == :alias
        lit = exp.last
        case lit
        when Integer
          exp.new(:nat_integer, :env, lit)
        when Symbol
          exp.new(:nat_symbol, :env, s(:s, lit))
        else
          raise "unknown lit: #{exp.inspect}"
        end
      end

      def rewrite_lvar(exp)
        return exp if context.first == :defined
        (_, name) = exp
        exp.new(:nat_lookup, :env, s(:s, name))
      end
      
      def rewrite_masgn(exp)
        if context.detect { |c| c != :masgn } == :args
          return exp.new(:masgn_in_args_full, *exp[1..-1])
        end
        (_, names, vals) = exp
        vars = []
        names[1..-1].each do |e|
          case e.sexp_type
          when :lasgn, :iasgn
            vars << s(:nat_symbol, :env, s(:s, e.last))
            vars << s(:nil)
          else
            vars << e
            vars << s(:nil)
          end
        end
        vars = exp.new(:array, *vars)
        context.push(:nat_masgn)
        vars = rewrite(vars)
        context.pop
        if vals
          raise "unknown masgn val type: #{vals.sexp_type}" unless vals.sexp_type == :to_ary
          exp.new(:nat_multi_assign, :env, :self, vars, vals.last)
        else
          vars
        end
      end

      def rewrite_masgn_in_args_full(exp)
        (_, *names) = exp
        args = []
        names.each do |name|
          case name
          when Symbol
            args << s(:nat_symbol, :env, s(:s, name))
            args << s(:NULL)
          when Sexp
            case name.sexp_type
            when :shadow
              puts "warning: unhandled arg type: #{name.inspect}"
            when :env_set
              (_, _, n, v) = name
              args << s(:nat_symbol, :env, n)
              args << v
            else
              args << name
              args << s(:NULL)
            end
          when nil
            # we need this so nat_multi_assign_args knows to spread the values out
            args << s(:NULL)
            args << s(:NULL)
          else
            raise "unknown arg type: #{name.inspect}"
          end
        end
        args = exp.new(:array, *args)
        context.push(:nat_masgn_in_args)
        args = rewrite(args)
        context.pop
        args
      end

      def rewrite_masgn_in_args_simple(exp)
        (_, *names) = exp
        args = names.each_with_index.map do |name, index|
          case name
          when Symbol
            if name.to_s.start_with?('*')
              s(:env_set, :env, s(:s, name.to_s[1..-1]), s(:block_arg, index, :rest))
            else
              s(:env_set, :env, s(:s, name), s(:block_arg, index, :single))
            end
          when Sexp
            case name.sexp_type
            when :shadow
              puts "warning: unhandled arg type: #{name.inspect}"
            when :env_set
              (_, _, (_, name), default) = name
              s(:env_set, :env, s(:s, name), s(:block_arg, index, :single, default))
            end
          when nil
            nil
          else
            raise "unknown arg type: #{name.inspect}"
          end
        end
        s(:block_args, *args)
      end

      def rewrite_module(exp)
        (_, name, *body) = exp
        fn = temp('module_body')
        mod = temp('module')
        exp.new(:block,
          s(:fn2, fn, rewrite(s(:block, *body))),
          s(:declare, mod, s(:env_get, :env, s(:s, name))),
          s(:c_if, s(:not, mod),
            s(:block,
              s(:set, mod, s(:nat_module, :env, s(:s, name))),
              s(:env_set, :env, s(:s, name), mod))),
          s(:nat_call, fn, "#{mod}->env", mod))
      end

      def rewrite_or(exp)
        (_, lhs, rhs) = exp
        exp.new(:c_if, s(:not, s(:nat_truthy, lhs)), rhs, lhs)
      end

      def rewrite_rescue(exp)
        (_, *rest) = exp
        else_body = rest.pop if rest.last.sexp_type != :resbody
        (body, resbodies) = rest.partition { |n| n.first != :resbody }
        begin_fn = temp('begin_fn')
        rescue_fn = begin_fn.sub(/begin/, 'rescue')
        rescue_block = s(:cond)
        resbodies.each_with_index do |(_, (_, *match), *resbody), index|
          lasgn = match.pop if match.last&.first == :env_set
          match << rewrite(s(:const, 'StandardError')) if match.empty?
          condition = s(:is_a, 'env->exception', *match)
          rescue_block << condition
          resbody = resbody == [nil] ? [s(:nil)] : resbody
          rescue_block << (lasgn ? s(:block, lasgn, *resbody) : s(:block, *resbody))
        end
        rescue_block << s(:else)
        rescue_block << s(:block, s(:nat_raise_exception, :env, 'env->exception'))
        if else_body
          body << s(:clear_jump_buf)
          body << else_body
        end
        body = body.empty? ? [s(:nil)] : body
        exp.new(:block,
          s(:fn2, begin_fn,
            s(:block,
              s(:build_block_env),
              s(:nat_rescue,
                s(:block, *body),
                rescue_block))),
          s(:nat_call, begin_fn, :env, :self))
      end

      def rewrite_sclass(exp)
        (_, obj, *body) = exp
        exp.new(:with_self, s(:nat_singleton_class, :env, rewrite(obj)),
          s(:block, *body))
      end

      def rewrite_str(exp)
        (_, str) = exp
        exp.new(:nat_string, :env, s(:s, str))
      end

      def rewrite_super(exp)
        (_, *args) = exp
        exp.new(:nat_super, s(:args, *args))
      end

      def rewrite_while(exp)
        (_, condition, body, unknown) = exp
        raise 'check this out' if unknown != true # FIXME: I don't know what this is; it always seems to be true
        body ||= s(:nil)
        exp.new(:block,
          s(:c_while, 'TRUE',
            s(:block,
              s(:c_if, s(:not, s(:nat_truthy, rewrite(condition))), s(:break)),
              body)),
          s(:nil))
      end

      def rewrite_yield(exp)
        (_, *args) = exp
        if args.any? { |a| a.sexp_type == :splat }
          args = s(:args_array, rewrite(s(:array, *args)))
        else
          args = s(:args, *args)
        end
        exp.new(:nat_run_block, args)
      end

      def rewrite_zsuper(exp)
        exp.new(:nat_super, s(:args))
      end

      def temp(name)
        @var_num += 1
        "#{var_prefix}#{name}#{@var_num}"
      end

      def args_use_simple_mode?(names)
        by_type = names.map do |name|
          case name
          when Symbol
            if name.to_s.start_with?('*')
              '*'
            elsif name.to_s.start_with?('&')
              '' # block arg is always last and can be ignored
            else
              'R'
            end
          when Sexp
            case name.sexp_type
            when :lasgn, :env_set
              'D'
            else
              return false
            end
          end
        end.join
        return false if by_type =~ /\*./ # rest must be last
        return false if by_type =~ /DR/  # defaulted must come after required
        true
      end
    end
  end
end
