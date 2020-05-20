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
        @source_files = {}
        @source_methods = {}
        @top = []
        @decl = []
      end

      VOID_FUNCTIONS = %i[
        NAT_ASSERT_ARGC
        NAT_ASSERT_ARGC_AT_LEAST
        NAT_ASSERT_TYPE
        NAT_ASSERT_NOT_FROZEN
        NAT_UNREACHABLE
        nat_alias
        nat_array_expand_with_nil
        nat_array_push
        nat_array_push_splat
        nat_class_include
        nat_class_prepend
        nat_define_method
        nat_define_method_with_block
        nat_define_singleton_method
        nat_define_singleton_method_with_block
        nat_grow_array
        nat_grow_array_at_least
        nat_grow_string
        nat_grow_string_at_least
        nat_handle_top_level_exception
        nat_hash_key_list_remove_node
        nat_hash_put
        nat_methods
        nat_print_exception_with_backtrace
        nat_quicksort
        nat_run_at_exit_handlers
        nat_string_append
        nat_string_append_char
        nat_string_append_nat_string
        nat_undefine_method
        nat_undefine_singleton_method
      ]

      NAT_FUNCTIONS = /^nat_|eval\d+$/i

      def go(ast)
        result = process(ast)
        @compiler_context[:template]
          .sub('/*OBJ_NAT*/', obj_nat_declarations.join("\n"))
          .sub('/*OBJ_NAT_INIT*/', obj_nat_init_lines.join("\n"))
          .sub('/*TOP*/', top_matter)
          .sub('/*INIT*/', init_matter.to_s)
          .sub('/*BODY*/', @decl.join("\n") + "\n" + result)
      end

      def top_matter
        [
          source_files_to_c,
          source_methods_to_c,
          @top.join("\n")
        ].join("\n\n")
      end

      def init_matter
        return if @source_files.empty?
        "nat_global_set(env, \"$0\", nat_string(env, nat_source_files[0]));"
      end

      def process_atom(exp)
        case exp
        when Sexp
          process(exp)
        when String
          exp
        when Symbol, Integer, true, false
          exp.to_s
        else
          raise "unknown node type: #{exp.inspect}"
        end
      end

      def process_is_a(exp)
        (_, target, *list) = exp
        list.map do |klass|
          "nat_is_a(env, #{process_atom target}, #{process_atom klass})"
        end.join(' || ')
      end

      def process_args(exp)
        (_, *args) = exp
        case args.size
        when 0
          'NULL:0'
        when 1
          "&#{process_atom args.first}:1"
        else
          args_name = temp('args')
          decl "NatObject *#{args_name}[#{args.size}] = { #{args.map { |arg| process_atom(arg) }.join(', ')} };"
          "#{args_name}:#{args.size}"
        end
      end

      def process_args_array(exp)
        (_, args) = exp
        array = process_atom(args)
        name = temp('args_array')
        decl "NatObject *#{name} = #{array};"
        "(NatObject**)nat_vector_data(&#{name}->ary):nat_vector_size(&#{name}->ary)"
      end

      def process_block(exp)
        (_, *body) = exp
        body[0..-2].each do |node|
          result = process_atom(node)
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

      def process_assign(exp)
        (_, val, *args) = exp
        val = process(val)
        array_val = temp('array_val')
        decl "NatObject *#{array_val} = nat_to_ary(env, #{val}, false);"
        args.compact.each do |arg|
          arg = arg.dup
          arg_value = process_assign_val(arg.pop, "nat_vector_size(&#{array_val}->ary)", "(NatObject**)nat_vector_data(&#{array_val}->ary)")
          process(arg << arg_value)
        end
        val
      end

      def process_assign_args(exp)
        (_, *args) = exp
        if args.size > 1
          array_arg = temp('array_arg')
          decl 'if (argc == 1) {'
          decl "  NatObject *#{array_arg} = nat_to_ary(env, args[0], true);"
          args.compact.each do |arg|
            arg = arg.dup
            arg_value = process_assign_val(arg.pop, "nat_vector_size(&#{array_arg}->ary)", "(NatObject**)nat_vector_data(&#{array_arg}->ary)")
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
          decl "NatObject *#{rest} = nat_array(env);"
          decl "for (ssize_t i=#{index}; i<#{argc_name}; i++) {"
          decl "nat_array_push(env, #{rest}, #{args_name}[i]);"
          decl '}'
          rest
        else
          "#{index} < #{argc_name} ? #{args_name}[#{index}] : #{process_atom(default)}"
        end
      end

      def process_built_in_const(exp)
        exp.last
      end

      def process_c_assign(exp)
        (_, name, value) = exp
        decl "#{name} = #{process_atom value};"
        name
      end

      def process_c_break(_)
        decl 'break;'
        ''
      end

      def process_c_define_method(exp)
        (_, (_, name), (_, c)) = exp
        fn = temp('fn')
        top "NatObject *#{fn}(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {\n#{c}\n}"
        process(s(:nat_define_method, :env, :self, s(:s, name), fn))
        "nat_symbol(env, #{name.inspect})"
      end

      def process_c_eval(exp)
        (_, (_, c)) = exp
        decl c
        ''
      end

      def process_c_if(exp)
        (_, condition, true_body, false_body) = exp
        condition = process(condition)
        c = []
        result_name = temp('if')
        c << "NatObject *#{result_name};"
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

      def process_c_include(exp)
        (_, (_, lib)) = exp
        if lib.start_with?('<')
          top "#include #{lib}"
        else
          top "#include #{lib.inspect}"
        end
        ''
      end

      def process_c_return(exp)
        (_, value) = exp
        decl "return #{process_atom(value)};"
        ''
      end

      def process_c_top_eval(exp)
        (_, (_, c)) = exp
        top c
        ''
      end

      def process_c_while(exp)
        (_, condition, body) = exp
        condition = process_atom(condition)
        c = []
        in_decl_context do
          c << "while (#{condition}) {"
          result = process_atom(body)
          c += @decl
          c << '}'
        end
        decl c
        ''
      end

      def process_clear_jump_buf(_)
        decl 'env->rescue = false;'
        ''
      end

      def process_cond(exp)
        c = []
        count = 0
        result_name = temp('cond_result')
        in_decl_context do
          decl "NatObject *#{result_name} = NULL;"
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
        (_, name, value) = exp
        if value
          decl "NatObject *#{name} = #{process_atom value};"
        else
          decl "NatObject *#{name};"
        end
        name
      end

      def process_declare_block(exp)
        (_, name, value) = exp
        process_sexp(value, name, 'NatBlock')
        name
      end

      def process_defined(exp)
        (_, name) = exp
        result = temp('defined_result')
        case name.sexp_type
        when :const, :gvar
          decl "NatObject *#{result} = nat_defined_obj(env, self, #{name.last.to_s.inspect});"
        when :nat_send
          (_, receiver, name) = name
          receiver ||= 'self'
          decl "NatObject *#{result};"
          decl "if (!NAT_RESCUE(env)) {"
          decl "#{result} = nat_defined_obj(env, #{process_atom receiver}, #{name.to_s.inspect});"
          decl '} else {'
          decl "#{result} = #{process_atom s(:nil)};"
          decl '}'
        when :lit, :str
          decl "NatObject *#{result} = nat_string(env, \"expression\");"
        when :nil
          decl "NatObject *#{result} = nat_string(env, \"nil\");"
        else
          raise "unknown defined type: #{exp.inspect}"
        end
        result
      end

      def process_l(exp)
        (_, lit) = exp
        lit.to_s
      end

      def process_nat_env_set_method_name(exp)
        (_, name) = exp
        methods = "nat_#{@compiler_context[:var_prefix]}source_methods"
        index = @source_methods[name] ||= @source_methods.size
        decl "env->method_name = #{methods}[#{index}];"
        ''
      end

      def process_false(_)
        'NAT_FALSE'
      end

      def process_fn(exp, arg_list = 6)
        (_, name, body) = exp
        in_decl_context do
          result = process_atom(body)
          fn = []
          if arg_list == 6
            fn << "NatObject *#{name}(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {"
          elsif arg_list == 2
            fn << "NatObject *#{name}(NatEnv *env, NatObject *self) {"
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

      def process_fn2(exp)
        process_fn(exp, 2)
      end
      alias process_begin_fn process_fn2
      alias process_class_fn process_fn2
      alias process_module_fn process_fn2

      def process_nat_send(exp)
        debug_info(exp)
        (fn, receiver, method, args, block) = exp
        receiver_name = process_atom(receiver)
        args_name, args_count = process_atom(args).split(':')
        result_name = temp('call_result')
        decl "NatObject *#{result_name} = #{fn}(env, #{receiver_name}, #{method.to_s.inspect}, #{args_count}, #{args_name}, #{block || 'NULL'});"
        result_name
      end

      def process_nat_rescue(exp)
        (_, top, bottom) = exp
        c = []
        in_decl_context do
          c << "if (!NAT_RESCUE(env)) {"
          result = process_atom(top)
          c += @decl
          c << "return #{result};" unless result.empty?
          c << '} else {'
          c << 'nat_global_set(env, "$!", env->exception);'
          c << 'env->rescue = false;'
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
        (fn, args) = exp
        args_name, args_count = process_atom(args).split(':')
        result_name = temp('block_result')
        decl "NatObject *#{result_name} = NAT_RUN_BLOCK_FROM_ENV(env, #{args_count}, #{args_name});"
        result_name
      end

      def process_nat_super(exp)
        (_, args, block) = exp
        result_name = temp('call_result')
        if args.size > 1
          args_name, args_count = process_atom(args).split(':')
          decl "NatObject *#{result_name} = nat_call_method_on_class(env, NAT_OBJ_CLASS(self)->superclass, NAT_OBJ_CLASS(self)->superclass, nat_find_current_method_name(env), self, #{args_count}, #{args_name}, #{block || 'NULL'});"
        else
          decl "NatObject *#{result_name} = nat_call_method_on_class(env, NAT_OBJ_CLASS(self)->superclass, NAT_OBJ_CLASS(self)->superclass, nat_find_current_method_name(env), self, argc, args, #{block || 'NULL'});"
        end
        result_name
      end

      def process_nat_truthy(exp)
        (_, cond) = exp
        "nat_truthy(#{process_atom cond})"
      end

      def process_nil(_)
        'NAT_NIL'
      end

      def process_not(exp)
        (_, cond) = exp
        "!#{process_atom cond}"
      end

      def process_NULL(_)
        'NULL'
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
            "\\\\\\\\" # I'm in escape sequence hell
          when 127
            "\\#{byte}"
          when 32..255
            byte.chr
          else
            "\\#{byte}"
          end
        end
        '"' + c_chars.join + '"'
      end

      def process_self(_)
        'self'
      end

      def process_true(_)
        'NAT_TRUE'
      end

      def process_set(exp)
        (fn, name, value) = exp
        decl "#{name} = #{process_atom value};"
        ''
      end

      def process_sexp(exp, name = nil, type = 'NatObject')
        debug_info(exp)
        (fn, *args) = exp
        if fn !~ NAT_FUNCTIONS
          raise CompileError, "#{exp.inspect} not rewritten in pass 1 (#{exp&.file}##{exp&.line}, context: #{@context.inspect})"
        end
        if VOID_FUNCTIONS.include?(fn)
          decl "#{fn}(#{args.map { |a| process_atom(a) }.join(', ')});"
          ''
        else
          result_name = name || temp(fn)
          decl "#{type} *#{result_name} = #{fn}(#{args.map { |a| process_atom(a) }.join(', ')});"
          result_name
        end
      end

      def process_var_alloc(exp)
        count = exp.last
        if count > 0
          decl "env->vars = nat_vector(#{count});"
        end
        ''
      end

      def process_with_self(exp)
        (_, new_self, body) = exp
        self_was = temp('self_was')
        decl "NatObject *#{self_was} = self;"
        decl "self = #{process_atom new_self};"
        result = temp('sclass_result')
        decl "NatObject *#{result} = #{process_atom body};"
        decl "self = #{self_was};"
        result
      end

      def temp(name)
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
        files = "nat_#{@compiler_context[:var_prefix]}source_files"
        index = @source_files[exp.file] ||= @source_files.size
        line = "env->file = #{files}[#{index}]; env->line = #{exp.line || 0};"
        decl line unless @decl.last == line
      end

      def source_files_to_c
        files = "nat_#{@compiler_context[:var_prefix]}source_files"
        "char *#{files}[#{@source_files.size}] = { #{@source_files.keys.map(&:inspect).join(', ')} };"
      end

      def source_methods_to_c
        methods = "nat_#{@compiler_context[:var_prefix]}source_methods"
        "char *#{methods}[#{@source_methods.size}] = { #{@source_methods.keys.map(&:inspect).join(', ')} };"
      end

      def obj_nat_files
        Dir[File.expand_path('../../../src/*.nat', __dir__)].sort.map do |path|
          File.split(path).last.split('.').first
        end
      end

      def obj_nat_declarations
        obj_nat_files.map do |name|
          "NatObject *obj_nat_#{name}(NatEnv *env, NatObject *self);"
        end
      end

      def obj_nat_init_lines
        obj_nat_files.map do |name|
          "obj_nat_#{name}(env, self);"
        end
      end
    end
  end
end
