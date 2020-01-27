module Natalie
  class Compiler
    # process S-expressions from Ruby to C
    class Pass1 < SexpProcessor
      def initialize(compiler_context)
        super()
        self.require_empty = false
        @compiler_context = compiler_context
      end

      def go(ast)
        process(ast)
      end

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
        exp.new(:nat_send, process(receiver), method, args)
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
        receiver = receiver ? process(receiver) : :self
        if block_pass
          exp.new(:block,
            s(:declare, proc_name, process(block_pass)),
            s(:nat_send, receiver, method, args, "#{proc_name}->block"))
        else
          exp.new(:nat_send, receiver, method, args, 'NULL')
        end
      end

      def process_cdecl(exp)
        (_, name, value) = exp
        exp.new(:nat_const_set, :env, :self, s(:s, name), process(value))
      end

      def process_class(exp)
        (_, name, superclass, *body) = exp
        superclass ||= s(:const, 'Object')
        fn = temp('class_body')
        klass = temp('class')
        exp.new(:block,
          s(:class_fn, fn, process(s(:block, *body))),
          s(:declare, klass, s(:nat_const_get_or_null, :env, :self, s(:s, name))),
          s(:c_if, s(:not, klass),
            s(:block,
              s(:set, klass, s(:nat_subclass, :env, process(superclass), s(:s, name))),
              s(:nat_const_set, :env, :self, s(:s, name), klass))),
          s(:nat_call, fn, "#{klass}->env", klass))
      end

      def process_colon2(exp)
        (_, parent, name) = exp
        parent_name = temp('parent')
        exp.new(:block,
          s(:declare, parent_name, process(parent)),
          s(:nat_const_get, :env, parent_name, s(:s, name)))
      end

      def process_const(exp)
        (_, name) = exp
        exp.new(:nat_const_get, :env, :self, s(:s, name))
      end

      def process_defined(exp)
        (_, name) = exp
        name = process(name) if name.sexp_type == :call
        exp.new(:defined, name)
      end

      def process_defn_internal(exp)
        (_, name, (_, *args), *body) = exp
        name = name.to_s
        fn_name = temp('fn')
        if args.last&.to_s&.start_with?('&')
          block_arg = exp.new(:nat_var_set, :env, s(:s, args.pop.to_s[1..-1]), s(:nat_proc, :env, 'block'))
        end
        assign_args = if args_use_simple_mode?(args)
                        exp.new(:assign_args, *prepare_masgn_simple(args))
                      else
                        raise "We do not yet support complicated args like this, sorry! #{args.inspect}"
                      end
        exp.new(:def_fn, fn_name,
          s(:block,
            s(:nat_env_set_method_name, name),
            assign_args,
            block_arg || s(:block),
            process(s(:block, *body))))
      end

      def process_defn(exp)
        (_, name, args, *body) = exp
        fn = process_defn_internal(exp)
        exp.new(:block,
          fn,
          s(:nat_define_method, :self, s(:s, name), fn[1]),
          s(:nat_symbol, :env, s(:s, name)))
      end

      def process_defs(exp)
        (_, owner, name, args, *body) = exp
        fn = process_defn_internal(exp.new(:defs, name, args, *body))
        exp.new(:block,
          fn,
          s(:nat_define_singleton_method, :env, process(owner), s(:s, name), fn[1]),
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
        exp.new(:nat_global_set, :env, s(:s, name), process(value))
      end

      def process_gvar(exp)
        (_, name) = exp
        exp.new(:nat_global_get, :env, s(:s, name))
      end

      def process_hash(exp)
        (_, *pairs) = exp
        hash = temp('hash')
        inserts = pairs.each_slice(2).map do |(key, val)|
          s(:nat_hash_put, :env, hash, process(key), process(val))
        end
        exp.new(:block,
          s(:declare, hash, s(:nat_hash, :env)),
          s(:block, *inserts),
          hash)
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
        exp.new(:nat_ivar_set, :env, :self, s(:s, name), process(value))
      end

      def process_iter(exp)
        (_, call, (_, *args), *body) = exp
        block_fn = temp('block_fn')
        block = block_fn.sub(/_fn/, '')
        call = process(call)
        call[call.size-1] = block
        assign_args = if args_use_simple_mode?(args)
                        exp.new(:assign_args, *prepare_masgn_simple(args))
                      else
                        raise "We do not yet support complicated args like this, sorry! #{args.inspect}"
                      end
        exp.new(:block,
          s(:block_fn, block_fn,
            s(:block,
              s(:nat_env_set_method_name, '<block>'),
              assign_args,
              process(s(:block, *body)))),
          s(:declare_block, block, s(:nat_block, :env, :self, block_fn)),
          call)
      end

      def process_ivar(exp)
        (_, name) = exp
        exp.new(:nat_ivar_get, :env, :self, s(:s, name))
      end

      def process_lambda(exp)
        exp.new(:nat_lambda, :env, 'NULL') # note: the block gets overwritten by process_iter later
      end

      def process_lasgn(exp)
        (_, name, val) = exp
        exp.new(:nat_var_set, :env, s(:s, name), process(val))
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
        exp.new(:nat_var_get, :env, s(:s, name))
      end
      
      def process_masgn(exp)
        (_, names, val) = exp
        names = names[1..-1]
        val = val.last if val.sexp_type == :to_ary
        val_name = temp('assign_val')
        if args_use_simple_mode?(names)
          s(:assign, process(val), *prepare_masgn_simple(names))
        else
          raise "We do not yet support complicated args like this, sorry! #{names.inspect}"
        end
      end

      def prepare_masgn_simple(names)
        args = names.each_with_index.map do |name, index|
          case name
          when Symbol
            if name.to_s.start_with?('*')
              s(:nat_var_set, :env, s(:s, name.to_s[1..-1]), s(:assign_val, index, :rest))
            else
              s(:nat_var_set, :env, s(:s, name), s(:assign_val, index, :single))
            end
          when Sexp
            case name.sexp_type
            when :shadow
              puts "warning: unhandled arg type: #{name.inspect}"
            when :lasgn, :iasgn
              (_, name, default) = name
              s(:nat_var_set, :env, s(:s, name), s(:assign_val, index, :single, process(default)))
            when :splat
              (_, (_, name)) = name
              s(:nat_var_set, :env, s(:s, name), s(:assign_val, index, :rest, process(default)))
            else
              raise "unknown arg type: #{name.inspect}"
            end
          when nil
            nil
          else
            raise "unknown arg type: #{name.inspect}"
          end
        end
        args
      end

      def process_module(exp)
        (_, name, *body) = exp
        fn = temp('module_body')
        mod = temp('module')
        exp.new(:block,
          s(:module_fn, fn, process(s(:block, *body))),
          s(:declare, mod, s(:nat_const_get_or_null, :env, :self, s(:s, name))),
          s(:c_if, s(:not, mod),
            s(:block,
              s(:set, mod, s(:nat_module, :env, s(:s, name))),
              s(:nat_const_set, :env, :self, s(:s, name), mod))),
          s(:nat_call, fn, "#{mod}->env", mod))
      end

      def process_or(exp)
        (_, lhs, rhs) = exp
        lhs = process(lhs)
        rhs = process(rhs)
        exp.new(:c_if, s(:not, s(:nat_truthy, lhs)), rhs, lhs)
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
          s(:begin_fn, begin_fn,
            s(:block,
              s(:nat_build_block_env),
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
        n = @compiler_context[:var_num] += 1
        "#{@compiler_context[:var_prefix]}#{name}#{n}"
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
            when :lasgn, :nat_var_set
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
