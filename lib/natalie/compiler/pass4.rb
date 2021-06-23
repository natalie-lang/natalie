module Natalie
  class Compiler
    # Generate a C code from pseudo-C S-expressions
    class Pass4 < SexpProcessor
      def initialize(compiler_context)
        super()
        self.default_method = :process_sexp
        self.warn_on_default = false
        self.require_empty = false
        self.strict = true
        self.expected = String
        @compiler_context = compiler_context
        raise 'source path unknown!' unless compiler_context[:source_path]
        @source_files = { compiler_context[:source_path] => 0 }
        @symbols = {}
        @top = []
        @decl = []
      end

      VOID_FUNCTIONS = %i[
        NAT_ASSERT_NOT_FROZEN
        NAT_UNREACHABLE
        add_break_flag
        alias
        append
        array_expand_with_nil
        ensure_argc_between
        ensure_argc_is
        ensure_argc_at_least
        assert_type
        freeze
        grow_array
        grow_array_at_least
        grow_string
        grow_string_at_least
        handle_top_level_exception
        hash_key_list_remove_node
        include
        methods
        prepend
        print_exception_with_backtrace
        push
        push_splat
        put
        raise_exception
        raise_local_jump_error
        remove_break_flag
        run_at_exit_handlers
        undefine_method
        undefine_singleton_method
      ]

      TYPES = {
        as_class: 'ClassValue *',
        as_string: 'StringValue *',
      }

      METHODS = %i[
        alias
        append
        add_break_flag
        as_class
        as_string
        ensure_argc_between
        ensure_argc_is
        ensure_argc_at_least
        assert_type
        const_find
        const_get
        const_set
        cvar_get
        cvar_get_or_null
        cvar_set
        define_method
        define_singleton_method
        eval_body
        freeze
        get
        global_get
        global_set
        has_break_flag
        include
        ivar_get
        ivar_set
        last_match
        prepend
        public_send
        push
        push_splat
        put
        raise
        raise_exception
        raise_local_jump_error
        remove_break_flag
        send
        singleton_class
        subclass
        to_proc
        undefine_method
        undefine_singleton_method
        var_get
        var_set
      ]

      def go(ast)
        result = process(ast)
        out = @compiler_context[:template]
          .sub('/*' + 'NAT_DECLARATIONS' + '*/') { declarations }
          .sub('/*' + 'NAT_OBJ_INIT' + '*/') { obj_init_lines.join("\n") }
          .sub('/*' + 'NAT_EVAL_INIT' + '*/') { init_matter }
          .sub('/*' + 'NAT_EVAL_BODY' + '*/') { @decl.join("\n") + "\n" + result }
        reindent(out)
      end

      def declarations
        [
          obj_declarations,
          source_files_to_c,
          declare_symbols,
          @top.join("\n")
        ].join("\n\n")
      end

      def init_matter
        [
          init_symbols.join("\n"),
          set_dollar_zero_global_in_main_to_c,
        ].compact.join("\n")
      end

      def process___cxx_flags__(exp)
        (_, flags) = exp
        raise "Expected string passed to __cxx_flags__, but got: #{flags.inspect}" unless flags.sexp_type == :str
        @compiler_context[:compile_cxx_flags] << flags.last
        ''
      end

      def process___define_method__(exp)
        (_, name, args, body) = exp
        if exp.size < 4
          body = args
          args = nil
        end
        raise "Expected string passed to __define_method__, but got: #{body.inspect}" unless body.sexp_type == :str
        name = name.last.to_s
        c = []
        if args
          (_, *args) = args
          c << "env->ensure_argc_is(argc, #{args.size});"
          args.each_with_index do |arg, i|
            raise "Expected symbol arg name pass to __define_method__, but got: #{arg.inspect}" unless arg.sexp_type == :lit
            c << "ValuePtr #{arg.last} = args[#{i}];"
          end
        end
        c << body.last
        fn = temp('fn')
        nl = "\n" # FIXME: parser issue with double quotes inside interpolation
        top "ValuePtr #{fn}(Env *env, ValuePtr self, size_t argc, ValuePtr *args, Block *block) {\n#{c.join(nl)}\n}"
        process(s(:define_method, s(:l, "self->as_module()"), :env, s(:intern, name), fn, -1))
        "SymbolValue::intern(#{name.inspect})"
      end

      def process___inline__(exp)
        (_, body) = exp
        raise "Expected string passed to __inline__, but got: #{body.inspect}" unless body.sexp_type == :str
        body = body.last
        if context.grep(/_fn$/).any?
          decl body
        else
          top body
        end
        'NilValue::the()'
      end

      def process___ld_flags__(exp)
        (_, flags) = exp
        raise "Expected string passed to __ld_flags__, but got: #{flags.inspect}" unless flags.sexp_type == :str
        @compiler_context[:compile_ld_flags] << flags.last
        ''
      end

      def process_atom(exp)
        case exp
        when Sexp
          @last_sexp = exp
          process(exp)
        when String
          exp
        when Symbol, Integer, Float, true, false
          exp.to_s
        else
          raise "unknown node type: #{exp.inspect} (last sexp: #{@last_sexp.inspect})"
        end
      end

      def process_is_a(exp)
        (_, target, *list) = exp
        list.map do |klass|
          "#{process_atom target}->is_a(env, #{process_atom klass})"
        end.join(' || ')
      end

      def process_args(exp)
        (_, *args) = exp
        if args.size.zero?
          'nullptr:0'
        else
          args_name = temp('args')
          decl "ValuePtr #{args_name}[#{args.size}] = { #{args.map { |arg| process_atom(arg) }.join(', ')} };"
          "#{args_name}:#{args.size}"
        end
      end

      def process_args_array(exp)
        (_, args) = exp
        array = process_atom(args)
        name = temp('args_array')
        decl "ValuePtr #{name} = #{array};"
        "#{name}->as_array()->data():#{name}->as_array()->size()"
      end

      def process_block(exp)
        (_, *body) = exp
        body[0..-2].each do |node|
          process_atom(node)
        end
        result = if body.empty?
                   process(s(:nil))
                 else
                   process_atom(body.last)
                 end
        if context.size == 1
          result.empty? ? "return #{result};" : ''
        else
          result
        end
      end

      def process_and(exp)
        (_, left, right) = exp
        "#{process left} && #{process right}"
      end

      def process_assign(exp)
        (_, val, *args) = exp
        val = process(val)
        array_val = temp('array_val')
        decl "ValuePtr #{array_val} = to_ary(env, #{val}, false);"
        args.compact.each do |arg|
          arg = arg.dup
          arg_value = process_assign_val(arg.pop, "#{array_val}->size()", "#{array_val}->data()")
          process(arg << arg_value)
        end
        val
      end

      def process_assign_args(exp)
        (_, *args) = exp
        if args.size > 1
          array_arg = temp('array_arg')
          decl 'if (argc == 1) {'
          decl "  ValuePtr #{array_arg} = to_ary(env, args[0], true);"
          args.compact.each do |arg|
            arg = arg.dup
            arg_value = process_assign_val(arg.pop, "#{array_arg}->size()", "#{array_arg}->data()")
            process(arg << arg_value)
          end
          decl '} else {'
          args.compact.each { |a| process(a) }
          decl '}'
        else
          args.compact.each { |a| process(a) }
        end
        ''
      end

      def process_assign_val(exp, argc_name = 'argc', args_name = 'args')
        (_, index, type, default) = exp
        default ||= s(:nil)
        if type == :rest
          rest = temp('rest')
          decl "ArrayValue *#{rest} = new ArrayValue {};"
          decl "for (size_t i=#{index}; i<#{argc_name}; i++) {"
          decl "#{rest}->push(#{args_name}[i]);"
          decl '}'
          rest
        else
          "#{index} < #{argc_name} ? #{args_name}[#{index}] : #{process_atom(default)}"
        end
      end

      def process_built_in_const(exp)
        exp.last
      end

      def process_c_break(_)
        decl 'break;'
        ''
      end

      def process_c_if(exp)
        (_, condition, true_body, false_body) = exp
        condition = process(condition)
        c = []
        result_name = temp('if')
        c << "ValuePtr #{result_name};"
        in_decl_context do
          c << "if (#{condition}) {"
          result = process_atom(true_body)
          c += @decl
          @decl = []
          c << "#{result_name} = #{result};" unless result.empty?
          if false_body
            c << '} else {'
            result = process_atom(false_body)
            c += @decl
            c << "#{result_name} = #{result};" unless result.empty?
          end
          c << '}'
        end
        decl c
        result_name
      end

      def process_c_return(exp)
        (_, value) = exp
        decl "return #{process_atom(value)};"
        ''
      end

      def process_c_continue(_)
        decl 'continue;'
        ''
      end

      def process_c_do(exp)
        (_, body, condition) = exp
        condition = process_atom(condition)
        c = []
        in_decl_context do
          c << 'do {'
          process_atom(body)
          c += @decl
          c << "} while (#{condition});"
        end
        decl c
        ''
      end

      def process_c_not(exp)
        (_, cond) = exp
        "!#{process_atom cond}"
      end

      def process_c_while(exp)
        (_, condition, body) = exp
        condition = process_atom(condition)
        c = []
        in_decl_context do
          c << "while (#{condition}) {"
          process_atom(body)
          c += @decl
          c << '}'
        end
        decl c
        ''
      end

      def process_cond(exp)
        c = []
        count = 0
        result_name = temp('cond_result')
        in_decl_context do
          decl "ValuePtr #{result_name} = nullptr;"
          exp[1..-1].each_slice(2).each_with_index do |(cond, body), index|
            if cond == s(:else)
              in_decl_context do
                result = process_atom(body)
                c += @decl
                c << "#{result_name} = #{result};" unless result.empty?
              end
            else
              count += 1
              cond = process_atom(cond)
              c += @decl
              @decl = []
              in_decl_context do
                c << "if (#{cond}) {"
                result = process_atom(body)
                c += @decl
                c << "#{result_name} = #{result};" unless result.empty?
                c << '} else {'
              end
            end
          end
          count.times { c << '}' }
        end
        decl c
        result_name
      end

      def process_declare(exp)
        (_, name, value, type) = exp
        type ||= 'ValuePtr'
        if value
          decl "#{type} #{name} = #{process_atom value};"
        else
          decl "#{type} #{name};"
        end
        name
      end

      def process_declare_block(exp)
        (_, name, value) = exp
        process_new(value, name, 'Block *')
        name
      end

      def process_defined(exp)
        (_, name) = exp
        result = temp('defined_result')
        case name.sexp_type
        when :const, :gvar, :ivar
          decl "ValuePtr #{result} = self->defined_obj(env, SymbolValue::intern(#{name.last.to_s.inspect}));"
        when :send, :public_send
          (_, receiver, name) = name
          receiver ||= 'self'
          decl "ValuePtr #{result};"
          decl 'try {'
          decl "#{result} = #{process_atom receiver}->defined_obj(env, #{process name}, true);"
          decl '} catch (ExceptionValue *) {'
          decl "#{result} = #{process_atom s(:nil)};"
          decl '}'
        when :colon2
          (_, namespace, name) = name
          raise "expected const" unless namespace.first == :const
          decl "ValuePtr #{result};"
          decl 'try {'
          decl "#{result} = GlobalEnv::the()->Object()->const_find(env, SymbolValue::intern(#{namespace.last.to_s.inspect}), Value::ConstLookupSearchMode::NotStrict)->defined_obj(env, SymbolValue::intern(#{name.to_s.inspect}), true);"
          decl '} catch (ExceptionValue *) {'
          decl "#{result} = #{process_atom s(:nil)};"
          decl '}'
        when :colon3
          (_, name) = name
          decl "ValuePtr #{result};"
          decl 'try {'
          decl "#{result} = GlobalEnv::the()->Object()->defined_obj(env, SymbolValue::intern(#{name.to_s.inspect}), true);"
          decl '} catch (ExceptionValue *) {'
          decl "#{result} = #{process_atom s(:nil)};"
          decl '}'
        when :lit, :str
          decl "ValuePtr #{result} = new StringValue { \"expression\" };"
        when :nil
          decl "ValuePtr #{result} = new StringValue { \"nil\" };"
        else
          raise "unknown defined type: #{exp.inspect}"
        end
        result
      end

      def process_l(exp)
        (_, lit) = exp
        lit.to_s
      end

      def process_false(_)
        'ValuePtr { FalseValue::the() }'
      end

      def process_fn(exp, arg_list = 6)
        (_, name, body) = exp
        in_decl_context do
          result = process_atom(body)
          fn = []
          if arg_list == 6
            fn << "ValuePtr #{name}(Env *env, ValuePtr self, size_t argc, ValuePtr *args, Block *block) {"
          elsif arg_list == 2
            fn << "ValuePtr #{name}(Env *env, ValuePtr self) {"
          else
            raise "unknown arg_list #{arg_list.inspect}"
          end
          fn += @decl
          fn << "return #{result};" unless result.empty?
          fn << '}'
          top fn
        end
        ''
      end
      alias process_block_fn process_fn
      alias process_def_fn process_fn
      alias process_begin_fn process_fn

      def process_fn2(exp)
        process_fn(exp, 2)
      end
      alias process_class_fn process_fn2
      alias process_module_fn process_fn2

      def process_public_send(exp)
        process_send(exp)
      end

      def process_send(exp)
        debug_info(exp)
        (fn, receiver, method, args, block) = exp
        receiver_name = process_atom(receiver)
        if args
          args_name, args_count = process_atom(args).split(':')
        else
          args_name = 'nullptr'
          args_count = 0
        end
        result_name = temp('call_result')
        decl "ValuePtr #{result_name} = #{receiver_name}.#{fn}(env, #{process method}, #{args_count}, #{args_name}, #{block || 'nullptr'});"
        result_name
      end

      def process_rescue(exp)
        (_, top, bottom) = exp
        c = []
        in_decl_context do
          c << 'try {'
          result = process_atom(top)
          c += @decl
          c << "return #{result};" unless result.empty?
          c << '} catch (ExceptionValue *exception) {'
          c << 'env->global_set(SymbolValue::intern("$!"), exception);'
          @decl = []
          result = process_atom(bottom)
          c += @decl
          c << "return #{result};" unless result.empty?
          c << '}'
        end
        decl c
        ''
      end

      def process_NAT_RUN_BLOCK_FROM_ENV(exp)
        (_, args) = exp
        args_name, args_count = process_atom(args).split(':')
        result_name = temp('block_result')
        decl "ValuePtr #{result_name} = NAT_RUN_BLOCK_FROM_ENV(env, #{args_count}, #{args_name});"
        result_name
      end

      def process_super(exp)
        (_, args, block) = exp
        result_name = temp('call_result')
        if args
          args_name, args_count = process_atom(args).split(':')
          decl "ValuePtr #{result_name} = super(env, self, #{args_count}, #{args_name}, #{block || 'nullptr'});"
        else
          decl "ValuePtr #{result_name} = super(env, self, argc, args, #{block || 'nullptr'});"
        end
        result_name
      end

      def process_is_truthy(exp)
        (_, cond) = exp
        "#{process_atom cond}->is_truthy()"
      end

      def process_nil(_)
        'ValuePtr { NilValue::the() }'
      end

      def process_s(exp)
        (_, string) = exp
        c_chars = string.to_s.bytes.map do |byte|
          case byte
          when 9
            "\\t"
          when 10
            "\\n"
          when 13
            "\\r"
          when 27
            "\\e"
          when 34
            "\\\""
          when 92
            "\\134"
          when 32..126
            byte.chr
          else
            "\\#{byte.to_s(8)}"
          end
        end
        '"' + c_chars.join + '"'
      end

      def process_self(_)
        'self'
      end

      def process_true(_)
        'ValuePtr { TrueValue::the() }'
      end

      def process_set(exp)
        (_, name, value) = exp
        decl "#{name} = #{process_atom value};"
        name
      end

      def process_new(exp, name = nil, type = 'ValuePtr ')
        (_, klass, *args) = exp
        name = name || temp('new')
        decl "#{type}#{name} = new #{klass} { #{args.map { |a| process_atom(a) }.join(', ') } };"
        name
      end

      def process_sexp(exp, name = nil, type = 'ValuePtr ')
        debug_info(exp)
        (fn, *args) = exp
        if VOID_FUNCTIONS.include?(fn)
          if METHODS.include?(fn)
            (obj, *args) = args
            decl "#{process_atom obj}->#{fn}(#{args.map { |a| process_atom(a) }.join(', ')});"
          else
            decl "#{fn}(#{args.map { |a| process_atom(a) }.join(', ')});"
          end
          ''
        else
          result_name = name || temp(fn)
          if METHODS.include?(fn)
            (obj, *args) = args
            type = TYPES[fn] || 'ValuePtr '
            decl "#{type}#{result_name} = #{process_atom obj}->#{fn}(#{args.map { |a| process_atom(a) }.join(', ')});"
          else
            decl "#{type}#{result_name} = #{fn}(#{args.map { |a| process_atom(a) }.join(', ')});"
          end
          result_name
        end
      end

      def process_var_alloc(exp)
        count = exp.last
        if count > 0
          decl "env->build_vars(#{count});"
        end
        ''
      end

      def process_with_self(exp)
        (_, new_self, body) = exp
        self_was = temp('self_was')
        decl "ValuePtr #{self_was} = self;"
        decl "self = #{process_atom new_self};"
        result = temp('sclass_result')
        decl "ValuePtr #{result} = #{process_atom body};"
        decl "self = #{self_was};"
        result
      end

      def symbols_var_name
        "#{@compiler_context[:var_prefix]}symbols"
      end

      def process_intern(exp)
        (_, name) = exp
        @symbols[name.to_s] ||= @symbols.size
        "#{symbols_var_name}[#{@symbols[name.to_s]}]/*:#{name.to_s.gsub(/\*\/|\\/, '?')}*/"
      end

      def process_intern_value_ptr(exp)
        temp_name = temp('symbol')
        decl "ValuePtr #{temp_name} = #{process_intern(exp)};"
        temp_name
      end

      def temp(name)
        name = name.to_s.gsub(/[^a-zA-Z]/, '')
        n = @compiler_context[:var_num] += 1
        "#{@compiler_context[:var_prefix]}#{name}#{n}"
      end

      def top(code)
        Array(code).each do |line|
          @top << line
        end
      end

      def decl(code)
        Array(code).each do |line|
          @decl << line
        end
      end

      def in_decl_context
        decl_was = @decl
        @decl = []
        yield
        @decl = decl_was
      end

      def debug_info(exp)
        return unless exp.file
        files = "#{@compiler_context[:var_prefix]}source_files"
        index = @source_files[exp.file] ||= @source_files.size
        line = "env->set_file(#{files}[#{index}]); env->set_line(#{exp.line || 0});"
        decl line unless @decl.last == line
      end

      def source_files_to_c
        files = "#{@compiler_context[:var_prefix]}source_files"
        "const char *#{files}[#{@source_files.size}] = { #{@source_files.keys.map(&:inspect).join(', ')} };"
      end

      def declare_symbols
        "SymbolValue *#{symbols_var_name}[#{@symbols.size}] = {};"
      end

      def init_symbols
        @symbols.map do |name, index|
          "#{symbols_var_name}[#{index}] = SymbolValue::intern(#{name.to_s.inspect});"
        end
      end

      def set_dollar_zero_global_in_main_to_c
        return if @compiler_context[:is_obj]
        "env->global_set(SymbolValue::intern(\"$0\"), new StringValue { #{@compiler_context[:source_path].inspect} });"
      end

      def obj_files
        rb_files = Dir.children(File.expand_path('../../../src', __dir__)).grep(/\.rb$/)
        list = rb_files.sort.map do |name|
          name.split('.').first
        end
        ['exception'] + (list - ['exception'])
      end

      def obj_declarations
        obj_files.map do |name|
          "ValuePtr init_obj_#{name}(Env *env, ValuePtr self);"
        end.join("\n")
      end

      def obj_init_lines
        obj_files.map do |name|
          "init_obj_#{name}(env, self);"
        end
      end

      def reindent(code)
        out = []
        indent = 0
        code.split("\n").each do |line|
          indent -= 4 if line =~ /^\s*\}/
          indent = [0, indent].max
          out << line.sub(/^\s*/, ' ' * indent)
          indent += 4 if line.end_with?('{')
        end
        out.join("\n")
      end
    end
  end
end
