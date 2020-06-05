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
                s(:alias, :env, :self, s(:s, new_name), s(:s, old_name)),
                s(:nil))
      end

      def process_and(exp)
        (_, lhs, rhs) = exp
        lhs = process(lhs)
        rhs = process(rhs)
        exp.new(:c_if, s(:truthy, lhs), rhs, lhs)
      end

      def process_array(exp)
        (_, *items) = exp
        arr = temp('arr')
        items = items.map do |item|
          if item.sexp_type == :splat
            s(:array_push_splat, :env, arr, process(item.last))
          else
            s(:array_push, :env, arr, process(item))
          end
        end
        exp.new(:block,
                s(:declare, arr, s(:array_new, :env)),
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
        exp.new(:send, process(receiver), method, args)
      end

      def process_block(exp)
        (_, *parts) = exp
        exp.new(:block, *parts.map { |p| p.is_a?(Sexp) ? process(p) : p })
      end

      def process_break(exp)
        (_, value) = exp
        value ||= s(:nil)
        break_name = temp('break_value')
        exp.new(:block,
                s(:declare, break_name, process(value)),
                s(:flag_break, break_name),
                s(:c_return, break_name))
      end

      def process_call(exp, is_super: false)
        (_, receiver, method, *args) = exp
        return process_inline_c(exp) if receiver == s(:const, :C)
        (_, block_pass) = args.pop if args.last&.sexp_type == :block_pass
        if args.any? { |a| a.sexp_type == :splat }
          args = s(:args_array, process(s(:array, *args.map { |n| process(n) })))
        else
          args = s(:args, *args.map { |a| process(a) })
        end
        receiver = receiver ? process(receiver) : :self
        call = if is_super
                 if args.any?
                   exp.new(:super, args)
                 else
                   exp.new(:super, nil)
                 end
               else
                 exp.new(:send, receiver, method, args)
               end
        if block_pass
          proc_name = temp('proc_to_block')
          call << "#{proc_name}->as_proc()->block"
          exp.new(:block,
                  s(:declare, proc_name, s(:to_proc, :env, process(block_pass))),
                  call)
        else
          call << 'NULL'
          call
        end
      end

      def process_case(exp)
        (_, value, *whens, else_body) = exp
        value_name = temp('case_value')
        cond = s(:cond)
        whens.each do |when_exp|
          (_, (_, *matchers), *when_body) = when_exp
          when_body = when_body.map { |w| process(w) }
          when_body = [s(:nil)] if when_body == [nil]
          matchers.each do |matcher|
            cond << s(:truthy, s(:send, process(matcher), '===', s(:args, value_name), 'NULL'))
            cond << s(:block, *when_body)
          end
        end
        cond << s(:else)
        cond << process(else_body || s(:nil))
        exp.new(:block,
                s(:declare, value_name, process(value)),
                cond)
      end

      def process_cdecl(exp)
        (_, name, value) = exp
        exp.new(:const_set, :env, :self, s(:s, name), process(value))
      end

      def process_class(exp)
        (_, name, superclass, *body) = exp
        superclass ||= s(:const, 'Object')
        fn = temp('class_body')
        klass = temp('class')
        exp.new(:block,
                s(:class_fn, fn, process(s(:block, *body))),
                s(:declare, klass, s(:const_get_or_null, :env, :self, s(:s, name), s(:l, :true), s(:l, :true))),
                s(:c_if, s(:not, klass),
                  s(:block,
                    s(:set, klass, s(:subclass, :env, process(superclass), s(:s, name))),
                    s(:const_set, :env, :self, s(:s, name), klass))),
        s(:eval_class_or_module_body, :env, klass, fn))
      end

      def process_colon2(exp)
        (_, parent, name) = exp
        parent_name = temp('parent')
        exp.new(:block,
                s(:declare, parent_name, process(parent)),
                s(:const_get, :env, parent_name, s(:s, name), s(:l, :true)))
      end

      def process_colon3(exp)
        (_, name) = exp
        s(:const_get, :env, s(:l, :NAT_OBJECT), s(:s, name), s(:l, :true))
      end

      def process_const(exp)
        (_, name) = exp
        exp.new(:const_get, :env, :self, s(:s, name), s(:l, :false))
      end

      def process_cvdecl(exp)
        (_, name, value) = exp
        exp.new(:cvar_set, :env, :self, s(:s, name), process(value))
      end

      def process_cvasgn(exp)
        (_, name, value) = exp
        exp.new(:cvar_set, :env, :self, s(:s, name), process(value))
      end

      def process_cvar(exp)
        (_, name) = exp
        exp.new(:cvar_get, :env, :self, s(:s, name))
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
          block_arg = exp.new(:var_set, :env, s(:s, args.pop.to_s[1..-1]), s(:proc_new, :env, 'block'))
        end
        args_name = temp('args_as_array')
        assign_args = s(:block,
                        s(:declare, args_name, s(:args_to_array, :env, s(:l, 'argc'), s(:l, 'args'))),
                        *prepare_args(args, args_name))
        method_body = process(s(:block, *body))
        if raises_local_jump_error?(method_body)
          # We only need to wrap method body in a rescue for LocalJumpError if there is a `return` inside a block.
          method_body = s(:rescue,
                          method_body,
                          s(:cond,
                            s(:is_a, 'env->exception', process(s(:const, :LocalJumpError))),
                            process(s(:call, s(:l, 'env->exception'), :exit_value)),
                            s(:else),
                            s(:raise_exception, :env, 'env->exception')))
        end
        exp.new(:def_fn, fn_name,
                s(:block,
                  s(:env_set_method_name, name),
                  prepare_argc_assertion(args),
                  assign_args,
                  block_arg || s(:block),
                  method_body))
      end

      def process_defn(exp)
        (_, name, args, *body) = exp
        fn = process_defn_internal(exp)
        exp.new(:block,
                fn,
                s(:define_method, :env, :self, s(:s, name), fn[1]),
                s(:symbol, :env, s(:s, name)))
      end

      def process_defs(exp)
        (_, owner, name, args, *body) = exp
        fn = process_defn_internal(exp.new(:defs, name, args, *body))
        exp.new(:block,
                fn,
                s(:define_singleton_method, :env, process(owner), s(:s, name), fn[1]),
                s(:symbol, :env, s(:s, name)))
      end

      def process_dot2(exp)
        (_, beginning, ending) = exp
        exp.new(:range_new, :env, process(beginning), process(ending), 0)
      end

      def process_dot3(exp)
        (_, beginning, ending) = exp
        exp.new(:range_new, :env, process(beginning), process(ending), 1)
      end

      def process_dregx(exp)
        str_node = process_dstr(exp)
        str = str_node.pop
        str_node << exp.new(:regexp_new, :env, s(:l, "#{str}->as_string()->str"))
        str_node
      end

      def process_dstr(exp)
        (_, start, *rest) = exp
        string = temp('string')
        segments = rest.map do |segment|
          case segment.sexp_type
          when :evstr
            s(:string_append_string, :env, string, process(s(:call, segment.last, :to_s)))
          when :str
            s(:string_append, :env, string, s(:s, segment.last))
          else
            raise "unknown dstr segment: #{segment.inspect}"
          end
        end
        exp.new(:block,
                s(:declare, string, s(:string, :env, s(:s, start))),
                *segments,
                string)
      end

      def process_dsym(exp)
        str_node = process_dstr(exp)
        str = str_node.pop
        str_node << exp.new(:symbol, :env, s(:l, "#{str}->as_string()->str"))
        str_node
      end

      def process_ensure(exp)
        (_, body, ensure_body) = exp
        process(
          exp.new(:block,
                  s(:rescue,
                    body,
                    s(:resbody, s(:array),
                      ensure_body,
                      s(:raise_exception, :env, s(:l, 'env->exception')))),
        ensure_body))
      end

      def process_gasgn(exp)
        (_, name, value) = exp
        exp.new(:global_set, :env, s(:s, name), process(value))
      end

      def process_gvar(exp)
        (_, name) = exp
        if name == :$~
          exp.new(:last_match, :env)
        else
          exp.new(:global_get, :env, s(:s, name))
        end
      end

      def process_hash(exp)
        (_, *pairs) = exp
        hash = temp('hash')
        inserts = pairs.each_slice(2).map do |(key, val)|
          s(:hash_put, :env, hash, process(key), process(val))
        end
        exp.new(:block,
                s(:declare, hash, s(:hash_new, :env)),
                s(:block, *inserts),
                hash)
      end

      def process_if(exp)
        (_, condition, true_body, false_body) = exp
        condition = exp.new(:truthy, process(condition))
        exp.new(:c_if,
                condition,
                process(true_body || s(:nil)),
                process(false_body || s(:nil)))
      end

      def process_iasgn(exp)
        (_, name, value) = exp
        exp.new(:ivar_set, :env, :self, s(:s, name), process(value))
      end

      def process_inline_c(exp)
        (_, _, op, *args) = exp
        case op
        when :define_method
          (name, c) = args
          raise "Expected string passed to C.define_method, but got: #{c.inspect}" unless c.sexp_type == :str
          exp.new(:c_define_method, name, c)
        when :eval
          c = args.first
          raise "Expected string passed to C.eval, but got: #{c.inspect}" unless c.sexp_type == :str
          exp.new(:c_eval, c)
        when :include
          lib = args.first
          raise "Expected string passed to C.include, but got: #{lib.inspect}" unless lib.sexp_type == :str
          exp.new(:c_include, lib)
        when :top_eval
          c = args.first
          raise "Expected string passed to C.top_eval, but got: #{c.inspect}" unless c.sexp_type == :str
          exp.new(:c_top_eval, c)
        else
          raise "Unknown C operation #{op.inspect}"
        end
      end

      def process_iter(exp)
        (_, call, (_, *args), *body) = exp
        args = fix_unnecessary_nesting(args)
        if args.last&.to_s&.start_with?('&')
          block_arg = exp.new(:arg_set, :env, s(:s, args.pop.to_s[1..-1]), s(:proc_new, :env, 'block'))
        end
        block_fn = temp('block_fn')
        block = block_fn.sub(/_fn/, '')
        call = process(call)
        call[call.size-1] = block
        args_name = temp('args_as_array')
        assign_args = s(:block,
                        s(:declare, args_name, s(:block_args_to_array, :env, args.size, s(:l, 'argc'), s(:l, 'args'))),
                        *prepare_args(args, args_name))
        exp.new(:block,
                s(:block_fn, block_fn,
                  s(:block,
                    s(:env_set_method_name, '<block>'),
                    assign_args,
                    block_arg || s(:block),
                    process(s(:block, *body)))),
        s(:declare_block, block, s(:block_new, :env, :self, block_fn)),
        call)
      end

      # |(a, b)| should be treated as |a, b|
      def fix_unnecessary_nesting(args)
        if args.size == 1 && args.first.is_a?(Sexp) && args.first.sexp_type == :masgn
          args.first[1..-1]
        else
          args
        end
      end

      def process_ivar(exp)
        (_, name) = exp
        exp.new(:ivar_get, :env, :self, s(:s, name))
      end

      def process_lambda(exp)
        exp.new(:lambda, :env, 'NULL') # note: the block gets overwritten by process_iter later
      end

      def process_lasgn(exp)
        (_, name, val) = exp
        exp.new(:var_set, :env, s(:s, name), process(val))
      end

      def process_lit(exp)
        lit = exp.last
        case lit
        when Integer
          exp.new(:integer, :env, lit)
        when Range
          exp.new(:range_new, :env, process_lit(s(:lit, lit.first)), process_lit(s(:lit, lit.last)), lit.exclude_end? ? 1 : 0)
        when Regexp
          exp.new(:regexp_new, :env, s(:s, lit.inspect[1...-1]))
        when Symbol
          exp.new(:symbol, :env, s(:s, lit))
        else
          raise "unknown lit: #{exp.inspect} (#{exp.file}\##{exp.line})"
        end
      end

      def process_lvar(exp)
        (_, name) = exp
        exp.new(:var_get, :env, s(:s, name))
      end

      def process_masgn(exp)
        (_, names, val) = exp
        names = names[1..-1]
        val = val.last if val.sexp_type == :to_ary
        value_name = temp('masgn_value')
        s(:block,
          s(:declare, value_name, s(:to_ary, :env, process(val), s(:l, :false))),
          *prepare_masgn(exp, value_name))
      end

      def prepare_masgn(exp, value_name)
        prepare_masgn_paths(exp).map do |name, path_details|
          path = path_details[:path]
          if name.is_a?(Sexp)
            if name.sexp_type == :splat
              value = s(:array_value_by_path, :env, value_name, s(:nil), s(:l, :true), path_details[:offset_from_end], path.size, *path)
              prepare_masgn_set(name.last, value)
            else
              default_value = name.size == 3 ? process(name.pop) : s(:nil)
              value = s(:array_value_by_path, :env, value_name, default_value, s(:l, :false), 0, path.size, *path)
              prepare_masgn_set(name, value)
            end
          else
            raise "unknown masgn type: #{name.inspect} (#{exp.file}\##{exp.line})"
          end
        end
      end

      def prepare_argc_assertion(args)
        min = 0
        max = 0
        has_kwargs = false
        args.each do |arg|
          if arg.is_a?(Symbol)
            if arg.start_with?('*')
              max = :unlimited
            else
              min += 1
              max += 1 if max != :unlimited
            end
          elsif arg.sexp_type == :kwarg
            max += 1 unless has_kwargs # FIXME: incrementing max here is wrong; it produces the wrong error message
            has_kwargs = true
          else
            max += 1 if max != :unlimited
          end
        end
        if max == :unlimited
          if min.zero?
            s(:block)
          else
            s(:NAT_ASSERT_ARGC_AT_LEAST, min)
          end
        elsif min == max
          s(:NAT_ASSERT_ARGC, min)
        else
          s(:NAT_ASSERT_ARGC, min, max)
        end
      end

      def prepare_args(names, value_name)
        names = prepare_arg_names(names)
        args_have_default = names.map { |e| %i[iasgn lasgn].include?(e.sexp_type) && e.size == 3 }
        defaults = args_have_default.select { |d| d }
        defaults_on_right = defaults.any? && args_have_default.uniq == [false, true]
        prepare_masgn_paths(s(:masgn, s(:array, *names))).map do |name, path_details|
          path = path_details[:path]
          if name.is_a?(Sexp)
            case name.sexp_type
            when :splat
              value = s(:arg_value_by_path, :env, value_name, s(:nil), s(:l, :true), names.size, defaults.size, defaults_on_right ? s(:l, :true) : s(:l, :false), path_details[:offset_from_end], path.size, *path)
              prepare_masgn_set(name.last, value, arg: true)
            when :kwarg
              default_value = name[2] ? process(name[2]) : 'NULL'
              value = s(:kwarg_value_by_name, :env, value_name, s(:s, name[1]), default_value)
              prepare_masgn_set(name, value, arg: true)
            else
              default_value = name.size == 3 ? process(name.pop) : s(:nil)
              value = s(:arg_value_by_path, :env, value_name, default_value, s(:l, :false), names.size, defaults.size, defaults_on_right ? s(:l, :true) : s(:l, :false), 0, path.size, *path)
              prepare_masgn_set(name, value, arg: true)
            end
          else
            raise "unknown masgn type: #{name.inspect} (#{exp.file}\##{exp.line})"
          end
        end
      end

      def prepare_arg_names(names)
        names.map do |name|
          case name
          when Symbol
            case name.to_s
            when /^\*@(.+)/
              s(:splat, s(:iasgn, name[1..-1].to_sym))
            when /^\*(.+)/
              s(:splat, s(:lasgn, name[1..-1].to_sym))
            when /^\*/
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
              s(:masgn, s(:array, *prepare_arg_names(name[1..-1])))
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

      def prepare_masgn_set(exp, value, arg: false)
        case exp.sexp_type
        when :cdecl
          s(:const_set, :env, :self, s(:s, exp.last), value)
        when :gasgn
          s(:global_set, :env, s(:s, exp.last), value)
        when :iasgn
          s(:ivar_set, :env, :self, s(:s, exp.last), value)
        when :lasgn, :kwarg
          if arg
            s(:arg_set, :env, s(:s, exp[1]), value)
          else
            s(:var_set, :env, s(:s, exp[1]), value)
          end
        else
          raise "unknown masgn type: #{exp.inspect} (#{exp.file}\##{exp.line})"
        end
      end

      # Ruby blows the stack at around this number, so let's limit Natalie as well.
      # Anything over a few dozen is pretty crazy, actually.
      MAX_MASGN_PATH_INDEX = 131_044

      def prepare_masgn_paths(exp, prefix = [])
        (_, (_, *names)) = exp
        splatted = false
        names.each_with_index.each_with_object({}) do |(e, index), hash|
          raise 'destructuring assignment is too big' if index > MAX_MASGN_PATH_INDEX
          has_default = %i[iasgn lasgn].include?(e.sexp_type) && e.size == 3
          if e.is_a?(Sexp) && e.sexp_type == :masgn
            hash.merge!(prepare_masgn_paths(e, prefix + [index]))
          elsif e.sexp_type == :splat
            splatted = true
            hash[e] = { path: prefix + [index], offset_from_end: names.size - index - 1 }
          elsif splatted
            hash[e] = { path: prefix + [(names.size - index) * -1] }
          else
            hash[e] = { path: prefix + [index] }
          end
        end
      end

      def process_match2(exp)
        (_, regexp, string) = exp
        s(:send, process(regexp), "=~", s(:args, process(string)))
      end

      def process_match3(exp)
        (_, string, regexp) = exp
        s(:send, process(regexp), "=~", s(:args, process(string)))
      end

      def process_module(exp)
        (_, name, *body) = exp
        fn = temp('module_body')
        mod = temp('module')
        exp.new(:block,
                s(:module_fn, fn, process(s(:block, *body))),
                s(:declare, mod, s(:const_get_or_null, :env, :self, s(:s, name), s(:l, :true), s(:l, :true))),
                s(:c_if, s(:not, mod),
                  s(:block,
                    s(:set, mod, s(:module, :env, s(:s, name))),
                    s(:const_set, :env, :self, s(:s, name), mod))),
        s(:eval_class_or_module_body, :env, mod, fn))
      end

      def process_next(exp)
        (_, value) = exp
        value ||= s(:nil)
        s(:c_return, process(value))
      end

      def process_nth_ref(exp)
        (_, num) = exp
        match = temp('match')
        exp.new(:block,
                s(:declare, match, s(:last_match, :env)),
                s(:c_if, s(:truthy, match), s(:send, match, :[], s(:args, s(:integer, :env, num))), s(:nil)))
      end

      def process_op_asgn1(exp)
        (_, obj, (_, *key_args), op, *val_args) = exp
        val = temp('val')
        obj_name = temp('obj')
        if op == :"||"
          exp.new(:block,
                  s(:declare, obj_name, process(obj)),
                  s(:declare, val, s(:send, obj_name, :[], s(:args, *key_args.map { |a| process(a) }), 'NULL')),
                  s(:c_if, s(:truthy, val),
                    val,
                    s(:send, obj_name, :[]=,
                      s(:args,
                        *key_args.map { |a| process(a) },
                        *val_args.map { |a| process(a) }),
          'NULL')))
        else
          exp.new(:block,
                  s(:declare, obj_name, process(obj)),
                  s(:declare, val,
                    s(:send,
                      s(:send, obj_name, :[], s(:args, *key_args.map { |a| process(a) }), 'NULL'),
                      op,
                      s(:args, *val_args.map { |a| process(a) }),
                      'NULL')),
                     s(:send, obj_name, :[]=,
                       s(:args,
                         *key_args.map { |a| process(a) },
                         val),
                         'NULL'))
        end
      end

      def process_op_asgn_and(exp)
        process_op_asgn_bool(exp, condition: ->(result_name) { s(:not, s(:truthy, result_name)) })
      end

      def process_op_asgn_or(exp)
        process_op_asgn_bool(exp, condition: ->(result_name) { s(:truthy, result_name) })
      end

      def process_op_asgn_bool(exp, condition:)
        (_, (var_type, name), value) = exp
        case var_type
        when :cvar
          result_name = temp('cvar')
          exp.new(:block,
                  s(:declare, result_name, s(:cvar_get_or_null, :env, :self, s(:s, name))),
                  s(:c_if, condition.(result_name), result_name, process(value)))
        when :gvar
          result_name = temp('gvar')
          exp.new(:block,
                  s(:declare, result_name, s(:global_get, :env, s(:s, name))),
                  s(:c_if, condition.(result_name), result_name, process(value)))
        when :ivar
          result_name = temp('ivar')
          exp.new(:block,
                  s(:declare, result_name, s(:ivar_get, :env, :self, s(:s, name))),
                  s(:c_if, condition.(result_name), result_name, process(value)))
        when :lvar
          var = process(s(:lvar, name))
          exp.new(:block,
                  s(:var_declare, :env, s(:s, name)),
                  s(:c_if, s(:defined, s(:lvar, name)),
                    s(:c_if, condition.(var),
                      var,
                      process(value))))
        else
          raise "unknown op_asgn_or type: #{var_type.inspect}"
        end
      end

      def process_or(exp)
        (_, lhs, rhs) = exp
        lhs = process(lhs)
        rhs = process(rhs)
        exp.new(:c_if, s(:not, s(:truthy, lhs)), rhs, lhs)
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
        rescue_block << s(:block, s(:raise_exception, :env, 'env->exception'))
        if else_body
          body << s(:clear_jump_buf)
          body << else_body
        end
        body = body.empty? ? [s(:nil)] : body
        exp.new(:block,
                s(:begin_fn, begin_fn,
                  s(:block,
                    s(:rescue,
                      s(:block, *body.map { |n| process(n) }),
                      rescue_block))),
                     s(:call_begin, :env, :self, begin_fn))
      end

      def process_return(exp)
        (_, value) = exp
        enclosing = context.detect { |n| %i[defn defs iter].include?(n) }
        if enclosing == :iter
          exp.new(:raise_local_jump_error, :env, process(value), s(:s, "unexpected return"))
        else
          exp.new(:c_return, process(value) || s(:nil))
        end
      end

      def process_sclass(exp)
        (_, obj, *body) = exp
        exp.new(:with_self, s(:singleton_class, :env, process(obj)),
                s(:block, *body.map { |n| process(n) }))
      end

      def process_str(exp)
        (_, str) = exp
        exp.new(:string, :env, s(:s, str))
      end

      def process_super(exp)
        (_, *args) = exp
        process_call(exp.new(:call, nil, :super, *args), is_super: true)
      end

      def process_svalue(exp)
        (_, (type, value)) = exp
        raise "Unknown svalue type #{type.inspect}" unless type == :splat
        exp.new(:splat, :env, process(value))
      end

      def process_while(exp)
        (_, condition, body, unknown) = exp
        raise 'check this out' if unknown != true # NOTE: I don't know what this is; it always seems to be true
        body ||= s(:nil)
        exp.new(:block,
                s(:c_while, 'true',
                  s(:block,
                    s(:c_if, s(:not, s(:truthy, process(condition))), s(:c_break)),
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
        exp.new(:NAT_RUN_BLOCK_FROM_ENV, args)
      end

      def process_zsuper(exp)
        exp.new(:super, s(:args))
      end

      def temp(name)
        n = @compiler_context[:var_num] += 1
        "#{@compiler_context[:var_prefix]}#{name}#{n}"
      end

      def raises_local_jump_error?(exp, my_context: [])
        if exp.is_a?(Sexp)
          case exp.sexp_type
          when :raise_local_jump_error
            return true if my_context.include?(:block_fn)
          when :def_fn # method within a method (unusual, but allowed!)
            return false
          else
            my_context << exp.sexp_type
            return true if exp[1..-1].any? { |e| raises_local_jump_error?(e, my_context: my_context) }
            my_context.pop
          end
        end
        return false
      end
    end
  end
end
