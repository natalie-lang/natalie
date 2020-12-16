module Natalie
  class Compiler
    # process S-expressions from Ruby to C
    class Pass1 < SexpProcessor
      def initialize(compiler_context)
        super()
        self.require_empty = false
        @compiler_context = compiler_context
        @loop_context = []
        @retry_context = []
      end

      def go(ast)
        process(ast)
      end

      def process_alias(exp)
        (_, (_, new_name), (_, old_name)) = exp
        exp.new(:block,
                s(:alias, :self, :env, s(:s, new_name), s(:s, old_name)),
                s(:nil))
      end

      def process_and(exp)
        (_, lhs, rhs) = exp
        lhs = process(lhs)
        rhs = process(rhs)
        exp.new(:c_if, s(:is_truthy, lhs), rhs, lhs)
      end

      def process_array(exp)
        (_, *items) = exp
        arr = temp('arr')
        items = items.map do |item|
          if item.sexp_type == :splat
            s(:push_splat, s(:l, "#{arr}->as_array()"), :env, process(item.last))
          else
            s(:push, s(:l, "#{arr}->as_array()"), process(item))
          end
        end
        exp.new(:block,
                s(:declare, arr, s(:new, :ArrayValue, :env)),
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

      def process_back_ref(exp)
        (_, name) = exp
        raise "Unknown back ref: #{name.inspect}" unless name == :&
        match = temp('match')
        exp.new(:block,
                s(:declare, match, s(:last_match, :env)),
                s(:c_if, s(:is_truthy, match), s(:send, match, :to_s), s(:nil)))
      end

      # bare begin without rescue
      def process_begin(exp)
        (_, code_block) = exp
        process(code_block)
      end

      def process_block(exp)
        (_, *parts) = exp
        exp.new(:block, *parts.map { |p| p.is_a?(Sexp) ? process(p) : p })
      end

      VALID_BREAK_CONTEXT = %i[iter while until]
      BREAK_LOOPS = %i[while until]

      def process_break(exp)
        (_, value) = exp
        value ||= s(:nil)
        break_name = temp('break_value')
        container = context.detect { |e| VALID_BREAK_CONTEXT.include?(e) }
        unless container
          puts "#{exp.file}##{exp.line}"
          raise SyntaxError, "Invalid break"
        end
        if BREAK_LOOPS.include?(container)
          result_name = @loop_context.last
          raise "No proper loop context!" if result_name.nil?
          exp.new(:block,
                  s(:set, result_name, process(value)),
                  s(:c_break))
        else
          exp.new(:block,
                  s(:declare, break_name, process(value)),
                  s(:add_break_flag, break_name),
                  s(:c_return, break_name))
        end
      end

      def process_call(exp, is_super: false)
        (_, receiver, method, *args) = exp
        if %i[__inline__ __define_method__ __compile_flags__].include?(method) && @compiler_context[:inline_cpp_enabled]
          return exp.new(method, *args)
        end
        (_, block_pass) = args.pop if args.last&.sexp_type == :block_pass
        if args.any? { |a| a.sexp_type == :splat }
          args = s(:args_array, process(s(:array, *args.map { |n| process(n) })))
        else
          args = s(:args, *args.map { |a| process(a) })
        end
        receiver = receiver ? process(receiver) : :self
        if method == :block_given?
          return exp.new(:send, receiver, method, args, 'block')
        end
        call = if is_super == :zsuper
                 exp.new(:super, nil)
               elsif is_super
                 exp.new(:super, args)
               else
                 exp.new(:send, receiver, method, args)
               end
        if block_pass
          proc_name = temp('proc_to_block')
          call << "proc_to_block_arg(env, #{proc_name})"
          exp.new(:block,
                  s(:declare, proc_name, process(block_pass)),
                  call)
        else
          call << 'nullptr'
          call
        end
      end

      def process_case(exp)
        (_, value, *whens, else_body) = exp
        if value # matching against a value
          value_name = temp('case_value')
          cond = s(:cond)
          whens.each do |when_exp|
            (_, (_, *matchers), *when_body) = when_exp
            when_body = when_body.map { |w| process(w) }
            when_body = [s(:nil)] if when_body == [nil]
            matchers.each do |matcher|
              cond << s(:is_truthy, s(:send, process(matcher), '===', s(:args, value_name), 'nullptr'))
              cond << s(:block, *when_body)
            end
          end
          cond << s(:else)
          cond << process(else_body || s(:nil))
          exp.new(:block,
                  s(:declare, value_name, process(value)),
                  cond)
        else # just a glorified if/elsif
          cond = s(:cond)
          whens.each do |when_exp|
            (_, (_, *matchers), *when_body) = when_exp
            when_body = when_body.map { |w| process(w) }
            when_body = [s(:nil)] if when_body == [nil]
            matchers.each do |matcher|
              cond << s(:is_truthy, process(matcher))
              cond << s(:block, *when_body)
            end
          end
          cond << s(:else)
          cond << process(else_body || s(:nil))
          cond
        end
      end

      def process_cdecl(exp)
        (_, name, value) = exp
        exp.new(:const_set, :self, :env, s(:s, name), process(value))
      end

      def process_class(exp)
        (_, name, superclass, *body) = exp
        superclass ||= s(:const, 'Object')
        fn = temp('class_body')
        klass = temp('class')
        exp.new(:block,
                s(:class_fn, fn, process(s(:block, *body))),
                s(:declare, klass, s(:const_get, :self, s(:s, name))),
                s(:c_if, s(:c_not, klass),
                  s(:block,
                    s(:set, klass, s(:subclass, s(:as_class, process(superclass)), :env, s(:s, name))),
                    s(:const_set, :self, :env, s(:s, name), klass))),
        s(:eval_body, s(:l, "#{klass}->as_class()"), :env, fn))
      end

      def process_colon2(exp)
        (_, parent, name) = exp
        parent_name = temp('parent')
        exp.new(:block,
                s(:declare, parent_name, process(parent)),
                s(:const_find, parent_name, :env, s(:s, name)))
      end

      def process_colon3(exp)
        (_, name) = exp
        s(:const_find, s(:l, 'env->Object()'), :env, s(:s, name))
      end

      def process_const(exp)
        (_, name) = exp
        exp.new(:const_find, :self, :env, s(:s, name), s(:l, 'Value::ConstLookupSearchMode::NotStrict'))
      end

      def process_cvdecl(exp)
        (_, name, value) = exp
        exp.new(:cvar_set, :self, :env, s(:s, name), process(value))
      end

      def process_cvasgn(exp)
        (_, name, value) = exp
        exp.new(:cvar_set, :self, :env, s(:s, name), process(value))
      end

      def process_cvar(exp)
        (_, name) = exp
        exp.new(:cvar_get, :self, :env, s(:s, name))
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
          arg_name = args.pop.to_s[1..-1]
          block_arg = exp.new(:var_set, :env, s(:s, arg_name), s(:"ProcValue::from_block_maybe", :env, 'block'))
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
                            s(:is_a, :exception, process(s(:const, :LocalJumpError))),
                            process(s(:call, s(:l, :exception), :exit_value)),
                            s(:else),
                            s(:raise_exception, :env, :exception)))
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
                s(:define_method, :self, :env, s(:s, name), fn[1]),
                s(:"SymbolValue::intern", :env, s(:s, name)))
      end

      def process_defs(exp)
        (_, owner, name, args, *body) = exp
        fn = process_defn_internal(exp.new(:defs, name, args, *body))
        exp.new(:block,
                fn,
                s(:define_singleton_method, process(owner), :env, s(:s, name), fn[1]),
                s(:"SymbolValue::intern", :env, s(:s, name)))
      end

      def process_dot2(exp)
        (_, beginning, ending) = exp
        exp.new(:new, :RangeValue, :env, process(beginning), process(ending), 0)
      end

      def process_dot3(exp)
        (_, beginning, ending) = exp
        exp.new(:new, :RangeValue, :env, process(beginning), process(ending), 1)
      end

      def process_dregx(exp)
        str_node = process_dstr(exp)
        str = str_node.pop
        str_node << exp.new(:new, :RegexpValue, :env, s(:l, "#{str}->as_string()->c_str()"))
        str_node
      end

      # TODO: support /o option on Regexes which compiles the literal only once
      alias process_dregx_once process_dregx

      def process_dstr(exp)
        (_, start, *rest) = exp
        string = temp('string')
        segments = rest.map do |segment|
          case segment.sexp_type
          when :evstr
            s(:append_string, s(:as_string, string), :env, process(s(:call, segment.last, :to_s)))
          when :str
            s(:append, s(:as_string, string), :env, s(:s, segment.last))
          else
            raise "unknown dstr segment: #{segment.inspect}"
          end
        end
        exp.new(:block,
                s(:declare, string, s(:new, :StringValue, :env, s(:s, start))),
                *segments,
                string)
      end

      def process_dsym(exp)
        str_node = process_dstr(exp)
        str = str_node.pop
        str_node << exp.new(:l, "#{str}->as_string()->to_symbol(env)")
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
                      s(:raise_exception, :env, :exception))),
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
          s(:put, s(:l, "#{hash}->as_hash()"), :env, process(key), process(val))
        end
        exp.new(:block,
                s(:declare, hash, s(:new, :HashValue, :env)),
                s(:block, *inserts),
                hash)
      end

      def process_if(exp)
        (_, condition, true_body, false_body) = exp
        condition = exp.new(:is_truthy, process(condition))
        exp.new(:c_if,
                condition,
                process(true_body || s(:nil)),
                process(false_body || s(:nil)))
      end

      def process_iasgn(exp)
        (_, name, value) = exp
        exp.new(:ivar_set, :self, :env, s(:s, name), process(value))
      end

      def process_iter(exp)
        (_, call, (_, *args), *body) = exp
        args = fix_unnecessary_nesting(args)
        if args.last&.to_s&.start_with?('&')
          block_arg = exp.new(:arg_set, :env, s(:s, args.pop.to_s[1..-1]), s(:new, :ProcValue, :env, 'block'))
        end
        block_fn = temp('block_fn')
        block = block_fn.sub(/_fn/, '')
        call = process(call)
        if (i = call.index('nullptr'))
          call[i] = block
        else
          puts "#{exp.file}##{exp.line}: #{call.inspect}"
          raise "cannot add block to call!"
        end
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
        s(:declare_block, block, s(:new, :Block, s(:l, "*env"), :self, block_fn)),
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
        exp.new(:ivar_get, :self, :env, s(:s, name))
      end

      def process_lambda(exp)
        # note: the nullptr gets overwritten with an actual block by process_iter later
        exp.new(:new, :ProcValue, :env, 'nullptr', :"ProcValue::ProcType::Lambda")
      end

      def process_lasgn(exp)
        (_, name, val) = exp
        exp.new(:var_set, :env, s(:s, name), process(val))
      end

      def process_lit(exp)
        lit = exp.last
        case lit
        when Float
          exp.new(:new, :FloatValue, :env, lit)
        when Integer
          exp.new(:new, :IntegerValue, :env, lit)
        when Range
          exp.new(:new, :RangeValue, :env, process_lit(s(:lit, lit.first)), process_lit(s(:lit, lit.last)), lit.exclude_end? ? 1 : 0)
        when Regexp
          exp.new(:new, :RegexpValue, :env, s(:s, lit.source), lit.options)
        when Symbol
          exp.new(:"SymbolValue::intern", :env, s(:s, lit))
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
              if name.size == 1 # nameless splat
                s(:block)
              else
                value = s(:array_value_by_path, :env, value_name, s(:nil), s(:l, :true), path_details[:offset_from_end], path.size, *path)
                prepare_masgn_set(name.last, value)
              end
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
            s(:assert_argc_at_least, :env, :argc, min)
          end
        elsif min == max
          s(:assert_argc, :env, :argc, min)
        else
          s(:assert_argc, :env, :argc, min, max)
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
              default_value = name[2] ? process(name[2]) : 'nullptr'
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
          s(:const_set, :self, :env, s(:s, exp.last), value)
        when :gasgn
          s(:global_set, :env, s(:s, exp.last), value)
        when :iasgn
          s(:ivar_set, :self, :env, s(:s, exp.last), value)
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
                s(:declare, mod, s(:const_get, :self, s(:s, name))),
                s(:c_if, s(:c_not, mod),
                  s(:block,
                    s(:set, mod, s(:new, :ModuleValue, :env, s(:s, name))),
                    s(:const_set, :self, :env, s(:s, name), mod))),
        s(:eval_body, s(:l, "#{mod}->as_module()"), :env, fn))
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
                s(:c_if, s(:is_truthy, match), s(:send, match, :[], s(:args, s(:new, :IntegerValue, :env, num))), s(:nil)))
      end

      def process_not(exp)
        (_, value) = exp
        exp.new(:send, process(value), :!, s(:args))
      end

      def process_op_asgn1(exp)
        (_, obj, (_, *key_args), op, *val_args) = exp
        val = temp('val')
        obj_name = temp('obj')
        if op == :"||"
          exp.new(:block,
                  s(:declare, obj_name, process(obj)),
                  s(:declare, val, s(:send, obj_name, :[], s(:args, *key_args.map { |a| process(a) }), 'nullptr')),
                  s(:c_if, s(:is_truthy, val),
                    val,
                    s(:send, obj_name, :[]=,
                      s(:args,
                        *key_args.map { |a| process(a) },
                        *val_args.map { |a| process(a) }
                       ),
                       'nullptr')))
        else # math operators
          exp.new(:block,
                  s(:declare, obj_name, process(obj)),
                  s(:declare, val,
                    s(:send,
                      s(:send, obj_name, :[], s(:args, *key_args.map { |a| process(a) }), 'nullptr'),
                      op,
                      s(:args, *val_args.map { |a| process(a) }),
                      'nullptr')),
                     s(:send, obj_name, :[]=,
                       s(:args,
                         *key_args.map { |a| process(a) },
                         val),
                         'nullptr'))
        end
      end

      def process_op_asgn2(exp)
        (_, obj, writer, op, *val_args) = exp
        raise "expected writer=" unless writer =~ /=$/
        reader = writer.to_s.chop
        val = temp('val')
        obj_name = temp('obj')
        if op == :"||"
          exp.new(:block,
                  s(:declare, obj_name, process(obj)),
                  s(:declare, val, s(:send, obj_name, reader, s(:args))),
                  s(:c_if, s(:is_truthy, val),
                    val,
                    s(:send, obj_name, writer,
                      s(:args,
                        *val_args.map { |a| process(a) }
                       ))))
        else
          exp.new(:block,
                  s(:declare, obj_name, process(obj)),
                  s(:declare, val, s(:send, s(:send, obj_name, reader, s(:args)), op, s(:args, *val_args.map { |a| process(a) }))),
                  s(:send, obj_name, writer, s(:args, val)))
        end
      end

      def process_op_asgn_and(exp)
        process_op_asgn_bool(exp, condition: ->(result_name) { s(:c_not, s(:is_truthy, result_name)) })
      end

      def process_op_asgn_or(exp)
        process_op_asgn_bool(exp, condition: ->(result_name) { s(:is_truthy, result_name) })
      end

      def process_op_asgn_bool(exp, condition:)
        (_, (var_type, name), value) = exp
        case var_type
        when :cvar
          result_name = temp('cvar')
          exp.new(:block,
                  s(:declare, result_name, s(:cvar_get_or_null, :self, :env, s(:s, name))),
                  s(:c_if, s(:and, s(:l, result_name), condition.(result_name)), result_name, process(value)))
        when :gvar
          result_name = temp('gvar')
          exp.new(:block,
                  s(:declare, result_name, s(:global_get, :env, s(:s, name))),
                  s(:c_if, condition.(result_name), result_name, process(value)))
        when :ivar
          result_name = temp('ivar')
          exp.new(:block,
                  s(:declare, result_name, s(:ivar_get, :self, :env, s(:s, name))),
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
        rhs = process(rhs)
        lhs_result = temp('lhs')
        exp.new(:block,
                s(:declare, lhs_result, process(lhs)),
                s(:c_if, s(:c_not, s(:is_truthy, lhs_result)), rhs, lhs_result))
      end

      def process_rescue(exp)
        (_, *rest) = exp
        retry_name = temp('should_retry')
        retry_context(retry_name) do
          else_body = rest.pop if rest.last.sexp_type != :resbody
          (body, resbodies) = rest.partition { |n| n.first != :resbody }
          begin_fn = temp('begin_fn')
          rescue_fn = begin_fn.sub(/begin/, 'rescue')
          rescue_block = s(:cond)
          resbodies.each_with_index do |(_, (_, *match), *resbody), index|
            lasgn = match.pop if match.last&.sexp_type == :lasgn
            match << s(:const, 'StandardError') if match.empty?
            condition = s(:is_a, :exception, *match.map { |n| process(n) })
            rescue_block << condition
            resbody = resbody == [nil] ? [s(:nil)] : resbody.map { |n| process(n) }
            rescue_block << (lasgn ? s(:block, process(lasgn), *resbody) : s(:block, *resbody))
          end
          rescue_block << s(:else)
          rescue_block << s(:block, s(:raise_exception, :env, :exception))
          if else_body
            body << else_body
          end
          body = body.empty? ? [s(:nil)] : body
          exp.new(:block,
                  s(:begin_fn, begin_fn,
                    s(:block,
                      s(:declare, retry_name, s(:l, :false), :bool),
                      s(:c_do,
                        s(:block,
                          s(:set, retry_name, s(:l, :false)),
                          s(:rescue,
                            s(:block, *body.map { |n| process(n) }),
                            rescue_block)),
                        retry_name),
                       s(:NAT_UNREACHABLE))),
                  s(:call_begin, :env, :self, begin_fn, :argc, :args, :block))
        end
      end

      def process_retry(_)
        retry_name = @retry_context.last
        raise "No proper rescue context!" if retry_name.nil?
        s(:block,
          s(:set, retry_name, s(:l, :true)),
          s(:c_continue))
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

      def process_safe_call(exp)
        (_, receiver, method, *args) = exp
        obj = temp('safe_call_obj')
        exp.new(:block,
                s(:declare, obj, process(receiver)),
                s(:c_if, s(:is_truthy, obj),
                  process(exp.new(:call, receiver, method, *args)),
                  s(:nil)))
      end

      def process_sclass(exp)
        (_, obj, *body) = exp
        exp.new(:with_self, s(:singleton_class, process(obj), :env),
                s(:block, *body.map { |n| process(n) }))
      end

      def process_str(exp)
        (_, str) = exp
        exp.new(:new, :StringValue, :env, s(:s, str))
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

      def process_until(exp)
        (_, condition, body, pre) = exp
        body ||= s(:nil)
        result_name = temp('until_result')
        loop_context(result_name) do
          cond = s(:c_if, s(:is_truthy, process(condition)), s(:c_break))
          loop_body = pre ? s(:block, cond, process(body)) : s(:block, process(body), cond)
          exp.new(:block,
                  s(:declare, result_name, s(:nil)),
                  s(:c_while, 'true', loop_body),
          result_name)
        end
      end

      def process_while(exp)
        (_, condition, body, pre) = exp
        body ||= s(:nil)
        result_name = temp('while_result')
        loop_context(result_name) do
          cond = s(:c_if, s(:c_not, s(:is_truthy, process(condition))), s(:c_break))
          loop_body = pre ? s(:block, cond, process(body)) : s(:block, process(body), cond)
          exp.new(:block,
                  s(:declare, result_name, s(:nil)),
                  s(:c_while, 'true', loop_body),
          result_name)
        end
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
        process_call(exp.new(:call, nil, :super), is_super: :zsuper)
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

      def loop_context(name)
        @loop_context << name
        exp = yield
        @loop_context.pop
        exp
      end

      def retry_context(name)
        @retry_context << name
        exp = yield
        @retry_context.pop
        exp
      end
    end
  end
end
