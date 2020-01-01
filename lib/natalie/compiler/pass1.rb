module Natalie
  class Compiler
    # Rewrite S-expressions from Ruby to C
    class Pass1 < SexpProcessor
      def initialize
        super
        self.require_empty = false
        @var_num = 0
      end

      attr_accessor :var_num, :var_prefix

      def process_alias(exp)
        (_, (_, new_name), (_, old_name)) = exp
        exp.new(:block,
          s(:nat_alias, :env, :self, s(:s, new_name), s(:s, old_name)),
          s(:nil))
      end

      def process_and(exp)
        (_, lhs, rhs) = exp
        lhs = process(lhs)
        rhs = process(rhs)
        exp.new(:c_if, s(:nat_truthy, lhs), rhs, lhs)
      end

      def process_array(exp)
        (_, *items) = exp
        arr = temp('arr')
        items = items.map do |item|
          if item.sexp_type == :splat
            s(:nat_array_push_splat, :env, arr, process(item.last))
          else
            s(:nat_array_push, arr, process(item))
          end
        end
        exp.new(:block,
          s(:declare, arr, s(:nat_array, :env)),
          *items.compact,
          arr)
      end

      def process_attrasgn(exp)
        (_, receiver, method, *args) = exp
        if args.any? { |a| a.sexp_type == :splat }
          args = s(:args_array, process(s(:array, *args.map { |n| process(n) })))
        else
          args = s(:args, *args.map { |n| process(n) })
        end
        exp.new(:nat_lookup_or_send, process(receiver), method, args)
      end

      def process_block(exp)
        (_, *parts) = exp
        exp.new(:block, *parts.map { |p| p.is_a?(Sexp) ? process(p) : p })
      end

      def process_call(exp)
        (_, receiver, method, *args) = exp
        (_, block_pass) = args.pop if args.last&.sexp_type == :block_pass
        if args.any? { |a| a.sexp_type == :splat }
          args = s(:args_array, process(s(:array, *args.map { |n| process(n) })))
        else
          args = s(:args, *args.map { |a| process(a) })
        end
        proc_name = temp('proc_to_block')
        sexp_type = receiver ? :nat_send : :nat_lookup_or_send
        receiver = receiver ? process(receiver) : :self
        if block_pass
          exp.new(:block,
            s(:declare, proc_name, block_pass),
            s(sexp_type, receiver, method, args, "#{proc_name}->block"))
        else
          exp.new(sexp_type, receiver, method, args, 'NULL')
        end
      end

      def process_cdecl(exp)
        (_, name, value) = exp
        exp.new(:env_set, :env, s(:s, name), process(value))
      end

      def process_class(exp)
        (_, name, superclass, *body) = exp
        superclass ||= s(:const, 'Object')
        fn = temp('class_body')
        klass = temp('class')
        exp.new(:block,
          s(:fn2, fn, process(s(:block, *body))),
          s(:declare, klass, s(:env_get, :env, s(:s, name))),
          s(:c_if, s(:not, klass),
            s(:block,
              s(:set, klass, s(:nat_subclass, :env, process(superclass), s(:s, name))),
              s(:env_set, :env, s(:s, name), klass))),
          s(:nat_call, fn, "#{klass}->env", klass))
      end

      def process_colon2(exp)
        (_, parent, name) = exp
        parent_name = temp('parent')
        exp.new(:block,
          s(:declare, parent_name, process(parent)),
          s(:env_get, "#{parent_name}->env", s(:s, name)))
      end

      def process_const(exp)
        (_, name) = exp
        exp.new(:nat_lookup, :env, s(:s, name))
      end

      def process_defined(exp)
        (_, name) = exp
        exp.new(:defined, name)
      end

      def process_defn(exp)
        (_, name, (_, *args), *body) = exp
        name = name.to_s
        fn = temp('fn')
        arg_names = args.map { |a| a.is_a?(Sexp) ? a[1] : a }
        arg_defaults = args.select { |a| a.is_a?(Sexp) && a.sexp_type == :lasgn }
        non_block_arg_count = arg_names.grep_v(/^\&/).size
        regular_arg_count = non_block_arg_count - arg_defaults.size
        splat_arg_index = arg_names.index { |a| a =~ /^\*/ }
        argc_assert = if splat_arg_index
                        s(:NAT_ASSERT_ARGC_AT_LEAST, splat_arg_index - arg_defaults.size)
                      elsif regular_arg_count == non_block_arg_count
                        s(:NAT_ASSERT_ARGC, regular_arg_count)
                      else
                        s(:NAT_ASSERT_ARGC, regular_arg_count, regular_arg_count + arg_defaults.size)
                      end
        exp.new(:block,
          s(:fn, fn,
            s(:block,
              argc_assert,
              s(:env_set, :env, s(:s, '__method__'), s(:nat_string, :env, s(:s, name))),
              s(:env_set_method_name, name),
              s(:block, *arg_defaults.map { |a| process(a) }),
              s(:method_args, *arg_names),
              process(s(:block, *body.map { |n| process(n) })))),
          s(:nat_define_method, :self, s(:s, name), fn),
          s(:nat_symbol, :env, s(:s, name)))
      end

      def process_defs(exp)
        (_, owner, name, (_, *args), *body) = exp
        name = name.to_s
        fn = temp('fn')
        non_block_args = args.grep_v(/^\&/)
        exp.new(:block,
          s(:fn, fn,
            s(:block,
              s(:NAT_ASSERT_ARGC, non_block_args.size),
              s(:env_set, :env, s(:s, '__method__'), s(:nat_string, :env, s(:s, name))),
              s(:method_args, *args),
              process(s(:block, *body)))),
          s(:nat_define_singleton_method, :env, process(owner), s(:s, name), fn),
          s(:nat_symbol, :env, s(:s, name)))
      end

      def process_dstr(exp)
        (_, start, *rest) = exp
        string = temp('string')
        segments = rest.map do |segment|
          case segment.sexp_type
          when :evstr
            s(:nat_string_append_nat_string, string, process(s(:call, segment.last, :to_s)))
          when :str
            s(:nat_string_append, string, s(:s, segment.last))
          else
            raise "unknown dstr segment: #{segment.inspect}"
          end
        end
        exp.new(:block,
          s(:declare, string, s(:nat_string, :env, s(:s, start))),
          *segments,
          string)
      end

      def process_gasgn(exp)
        (_, name, value) = exp
        exp.new(:global_set, :env, s(:s, name), process(value))
      end

      def process_gvar(exp)
        (_, name) = exp
        exp.new(:global_get, :env, s(:s, name))
      end

      def process_if(exp)
        (_, condition, true_body, false_body) = exp
        true_fn = temp('if_result_true')
        false_fn = true_fn.sub(/true/, 'false')
        exp.new(:block,
          s(:fn2, true_fn, process(true_body || s(:nil))),
          s(:fn2, false_fn, process(false_body || s(:nil))),
          s(:tern, process(condition), true_fn, false_fn))
      end

      def process_iasgn(exp)
        (_, name, value) = exp
        exp.new(:ivar_set, :env, :self, s(:s, name), process(value))
      end

      def process_iter(exp)
        (_, call, (_, *args), *body) = exp
        block_fn = temp('block_fn')
        block = block_fn.sub(/_fn/, '')
        call = process(call)
        call[call.size-1] = block
        exp.new(:block,
          s(:fn, block_fn,
            s(:block,
              s(:block_args, *args),
              process(s(:block, *body)))),
          s(:declare_block, block, s(:nat_block, :env, :self, block_fn)),
          call)
      end

      def process_ivar(exp)
        (_, name) = exp
        exp.new(:ivar_get, :env, :self, s(:s, name))
      end

      def process_lambda(exp)
        exp.new(:nat_lambda, :env, 'NULL') # note: the block gets overwritten by process_iter later
      end

      def process_lasgn(exp)
        (_, name, val) = exp
        exp.new(:env_set, :env, s(:s, name), process(val))
      end

      def process_lit(exp)
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

      def process_lvar(exp)
        (_, name) = exp
        exp.new(:nat_lookup, :env, s(:s, name))
      end
      
      def process_masgn(exp)
        (_, array, val) = exp
        raise 'expected s(:array, ...) for masgn target' if array.sexp_type != :array
        vars = array[1..-1].map do |var|
          if %i[lasgn iasgn].include?(var.sexp_type)
            s(:masgn_str, var[1])
          else
            raise 'expected iasgn or lasgn'
          end
        end
        raise 'expected s(:to_ary, ...) for masgn source' if val.sexp_type != :to_ary
        exp.new(:nat_multi_assign, s(:masgn_array, *vars), process(val[1]))
      end

      def process_module(exp)
        (_, name, *body) = exp
        fn = temp('module_body')
        mod = temp('module')
        exp.new(:block,
          s(:fn2, fn, process(s(:block, *body))),
          s(:declare, mod, s(:env_get, :env, s(:s, name))),
          s(:c_if, s(:not, mod),
            s(:block,
              s(:set, mod, s(:nat_module, :env, s(:s, name))),
              s(:env_set, :env, s(:s, name), mod))),
          s(:nat_call, fn, "#{mod}->env", mod))
      end

      def process_rescue(exp)
        (_, *rest) = exp
        else_body = rest.pop if rest.last.sexp_type != :resbody
        (body, resbodies) = rest.partition { |n| n.first != :resbody }
        begin_fn = temp('begin_fn')
        rescue_fn = begin_fn.sub(/begin/, 'rescue')
        rescue_block = s(:cond)
        resbodies.each_with_index do |(_, (_, *match), *resbody), index|
          lasgn = match.pop if match.last&.sexp_type == :lasgn
          match << s(:const, 'StandardError') if match.empty?
          condition = s(:is_a, 'env->exception', *match.map { |n| process(n) })
          rescue_block << condition
          resbody = resbody == [nil] ? [s(:nil)] : resbody.map { |n| process(n) }
          rescue_block << (lasgn ? s(:block, process(lasgn), *resbody) : s(:block, *resbody))
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
                s(:block, *body.map { |n| process(n) }),
                rescue_block))),
          s(:nat_call, begin_fn, :env, :self))
      end

      def process_sclass(exp)
        (_, obj, *body) = exp
        exp.new(:with_self, s(:nat_singleton_class, :env, process(obj)),
          s(:block, *body))
      end

      def process_str(exp)
        (_, str) = exp
        exp.new(:nat_string, :env, s(:s, str))
      end

      def process_super(exp)
        (_, *args) = exp
        exp.new(:nat_super, s(:args, *args.map { |n| process(n) }))
      end

      def process_while(exp)
        (_, condition, body, unknown) = exp
        raise 'check this out' if unknown != true # FIXME: I don't know what this is; it always seems to be true
        body ||= s(:nil)
        exp.new(:block,
          s(:c_while, 'TRUE',
            s(:block,
              s(:c_if, s(:not, s(:nat_truthy, process(condition))), s(:break)),
              process(body))),
          s(:nil))
      end

      def process_yield(exp)
        (_, *args) = exp
        if args.any? { |a| a.sexp_type == :splat }
          args = s(:args_array, process(s(:array, *args.map { |n| process(n) })))
        else
          args = s(:args, *args.map { |n| process(n) })
        end
        exp.new(:nat_run_block, args)
      end

      def process_zsuper(exp)
        exp.new(:nat_super, s(:args))
      end

      def temp(name)
        @var_num += 1
        "#{var_prefix}#{name}#{@var_num}"
      end
    end
  end
end
