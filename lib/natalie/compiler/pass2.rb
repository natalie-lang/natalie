module Natalie
  class Compiler
    # Generate a C code string from C S-expressions
    class Pass2 < SexpProcessor
      def initialize
        super
        self.default_method = :process_sexp
        self.warn_on_default = false
        self.require_empty = false
        self.strict = true
        self.expected = String
        @top = []
        @decl = []
        @var_num = 0
      end

      attr_accessor :var_num, :var_prefix

      VOID_FUNCTIONS = %i[
        nat_alias
        nat_array_push
        NAT_ASSERT_ARGC
        nat_define_method
        nat_raise_exception
        nat_string_append
        nat_string_append_nat_string
      ]

      NAT_FUNCTIONS = /^nat_|^NAT_|^env_|^ivar_|^global_/

      def p(exp)
        case exp
        when Sexp
          process(exp)
        when String
          exp
        when Symbol, Integer
          exp.to_s
        else
          raise "unknown node type: #{exp.inspect}"
        end
      end

      def process_is_a(exp)
        (_, target, *list) = exp
        list.map do |klass|
          "nat_is_a(env, #{p target}, #{p klass})"
        end.join(' || ')
      end

      def process_args(exp)
        (_, *args) = exp
        case args.size
        when 0
          'NULL:0'
        when 1
          "&#{p args.first}:1"
        else
          args_name = temp('args')
          decl "NatObject **#{args_name} = calloc(#{args.size}, sizeof(NatObject));"
          args.each_with_index do |arg, i|
            decl "#{args_name}[#{i}] = #{p arg};"
          end
          "#{args_name}:#{args.size}"
        end
      end

      def process_args_array(exp)
        (_, args) = exp
        array = p(args)
        name = temp('args_array')
        decl "NatObject *#{name} = #{array};"
        "#{name}->ary:#{name}->ary_len"
      end

      def process_block(exp)
        (_, *body) = exp
        body[0..-2].each do |node|
          result = p(node)
        end
        result = if body.empty?
                   'env_get(env, "nil")'
                 else
                   p(body.last)
                 end
        if context.size == 1
          result.empty? ? "return #{result};" : ''
        else
          result
        end
      end

      def process_block_args(exp)
        (_, *args) = exp
        if args.size == 1
          # DON'T attempt to spread array across args
          args.each_with_index do |arg, index|
            decl "nat_assign_arg(env, #{arg.to_s.inspect}, argc, args, #{index});"
          end
        else
          # DO attempt to spread array across args
          decl "if (argc > 0 && args[0]->type == NAT_VALUE_ARRAY) {"
          args.each_with_index do |arg, index|
            decl "nat_assign_arg(env, #{arg.to_s.inspect}, args[0]->ary_len, args[0]->ary, #{index});"
          end
          decl '} else {'
          args.each_with_index do |arg, index|
            decl "nat_assign_arg(env, #{arg.to_s.inspect}, argc, args, #{index});"
          end
          decl '}'
        end
        ''
      end

      def process_break(_)
        decl 'break;'
        ''
      end

      def process_build_block_env(_)
        decl "env = build_block_env(env, env);"
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
          result = p(true_body)
          c += @decl
          @decl = []
          c << "#{result_name} = #{result};" unless result.empty?
          if false_body
            c << '} else {'
            result = p(false_body)
            c += @decl
            c << "#{result_name} = #{result};" unless result.empty?
          end
          c << '}'
        end
        decl c
        result_name
      end

      def process_c_while(exp)
        (_, condition, body) = exp
        condition = p(condition)
        c = []
        in_decl_context do
          c << "while (#{condition}) {"
          result = p(body)
          c += @decl
          c << '}'
        end
        decl c
        ''
      end

      def process_clear_jump_buf(_)
        decl 'env->jump_buf = NULL;'
        ''
      end

      def process_cond(exp)
        c = []
        count = 0
        in_decl_context do
          exp[1..-1].each_slice(2).each_with_index do |(cond, body), index|
            if cond == s(:else)
              in_decl_context do
                result = p(body)
                c += @decl
                c << "return #{result};" unless result.empty?
              end
            else
              count += 1
              cond = p(cond)
              c += @decl
              @decl = []
              in_decl_context do
                c << "if (#{cond}) {"
                result = p(body)
                c += @decl
                c << "return #{result};" unless result.empty?
                c << '} else {'
              end
            end
          end
          count.times { c << '}' }
        end
        decl c
        ''
      end

      def process_declare(exp)
        (_, name, value) = exp
        process_sexp(value, name)
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
        when :const, :lvar
          decl "NatObject *#{result} = nat_defined_obj(env, self, #{name.last.to_s.inspect});"
        when :call
          # s(:call, nil, :y)
          (_, receiver, name) = name
          receiver ||= 'self'
          decl "NatObject *#{result} = nat_defined_obj(env, #{p receiver}, #{name.to_s.inspect});"
        else
          raise "unknown defined type: #{exp.inspect}"
        end
        result
      end

      def process_env_set_method_name(exp)
        (_, name) = exp
        decl "env->method_name = heap_string(#{name.inspect});"
        ''
      end

      def process_false(_)
        process_sexp(s(:env_get, :env, s(:s, 'false')), temp('false'))
      end

      def process_fn(exp, arg_list = 6)
        (_, name, body) = exp
        in_decl_context do
          result = p(body)
          fn = []
          if arg_list == 6
            fn << "NatObject *#{name}(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {"
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

      def process_fn2(exp)
        process_fn(exp, 2)
      end

      def process_method_args(exp)
        (_, *args) = exp
        args.each_with_index do |arg, index|
          if arg.to_s.start_with?('&')
            decl "env_set(env, #{arg.to_s[1..-1].inspect}, nat_proc(env, block));"
          elsif arg.to_s.start_with?('*')
            var = temp('args')
            decl "NatObject *#{var} = nat_array(env);"
            decl "for (size_t i=#{index}; i<argc; i++) {"
            decl "nat_array_push(#{var}, args[i]);"
            decl '}'
            decl "env_set(env, #{arg.to_s[1..-1].inspect}, #{var});"
          else
            decl "if (#{index} < argc) env_set(env, #{arg.to_s.inspect}, args[#{index}]);"
          end
        end
        ''
      end

      def process_nat_call(exp)
        (_, fn, *args) = exp
        if VOID_FUNCTIONS.include?(fn)
          decl "#{fn}(#{args.map { |a| p(a) }.join(', ')});"
          ''
        else
          result_name = temp(fn)
          decl "NatObject *#{result_name} = #{fn}(#{args.map { |a| p(a) }.join(', ')});"
          result_name
        end
      end

      def process_nat_lookup_or_send(exp)
        debug_info(exp)
        (fn, receiver, method, args, block) = exp
        receiver_name = p(receiver)
        args_name, args_count = p(args).split(':')
        result_name = temp('call_result')
        decl "NatObject *#{result_name} = #{fn}(env, #{receiver_name}, #{method.to_s.inspect}, #{args_count}, #{args_name}, #{block || 'NULL'});"
        result_name
      end

      def process_nat_multi_assign(exp)
        (fn, (_, *names), values) = exp
        values = p(values)
        decl "if (nat_respond_to(#{values}, \"to_ary\")) {"
        decl "NatObject *#{values}_array = nat_send(env, #{values}, \"to_ary\", 0, NULL, NULL);"
        decl "assert(#{values}_array->type == NAT_VALUE_ARRAY);"
        names.each_with_index do |name, index|
          raise "expected masgn_str for name, but got #{name}" unless name.sexp_type == :masgn_str
          name = name.last.to_s
          if name.start_with?('@')
            decl "ivar_set(env, self, #{name.inspect}, #{values}_array->ary[#{index}]);"
          else
            decl "env_set(env, #{name.inspect}, #{values}_array->ary[#{index}]);"
          end
        end
        decl "} else {"
        names.each_with_index do |name, index|
          raise "expected masgn_str for name, but got #{name}" unless name.sexp_type == :masgn_str
          name = name.last.to_s
          value = index == 0 ? values : 'env_get(env, "nil")'
          if name.start_with?('@')
            decl "ivar_set(env, self, #{name.inspect}, #{value});"
          else
            decl "env_set(env, #{name.inspect}, #{value});"
          end
        end
        decl '}'
        values
      end

      def process_nat_rescue(exp)
        (_, top, bottom) = exp
        c = []
        in_decl_context do
          c << "if (!NAT_RESCUE(env)) {"
          result = p(top)
          c += @decl
          c << "return #{result};" unless result.empty?
          c << '} else {'
          c << 'global_set(env, "$!", env->exception);'
          c << 'env->jump_buf = NULL;'
          @decl = []
          result = p(bottom)
          c += @decl
          c << "return #{result};" unless result.empty?
          c << "abort(); // not reached"
          c << '}'
        end
        decl c
        ''
      end

      def process_nat_run_block(exp)
        (fn, args) = exp
        args_name, args_count = p(args).split(':')
        result_name = temp('block_result')
        decl "NatObject *#{result_name} = nat_run_block(env, block, #{args_count}, #{args_name}, NULL, NULL);"
        result_name
      end

      alias process_nat_send process_nat_lookup_or_send

      def process_nat_super(exp)
        (_, args, block) = exp
        result_name = temp('call_result')
        if args
          args_name, args_count = p(args).split(':')
          decl "NatObject *#{result_name} = nat_call_method_on_class(env, self->class->superclass, self->class->superclass, env_get(env, \"__method__\")->str, self, #{args_count}, #{args_name}, NULL, #{block || 'NULL'});"
        else
          decl "NatObject *#{result_name} = nat_call_method_on_class(env, self->class->superclass, self->class->superclass, env_get(env, \"__method__\")->str, self, 0, NULL, NULL, #{block || 'NULL'});"
        end
        result_name
      end

      def process_nat_truthy(exp)
        (_, cond) = exp
        "nat_truthy(#{p cond})"
      end

      def process_nil(_)
        process_sexp(s(:env_get, :env, s(:s, 'nil')), temp('nil'))
      end

      def process_not(exp)
        (_, cond) = exp
        "!#{p cond}"
      end

      def process_s(exp)
        (_, string) = exp
        string.to_s.inspect
      end

      def process_self(_)
        'self'
      end

      def process_true(_)
        process_sexp(s(:env_get, :env, s(:s, 'true')), temp('true'))
      end

      def process_set(exp)
        (fn, name, value) = exp
        decl "#{name} = #{p value};"
        ''
      end

      def process_sexp(exp, name = nil, type = 'NatObject')
        debug_info(exp)
        (fn, *args) = exp
        if fn !~ NAT_FUNCTIONS
          raise RewriteError, "#{exp.inspect} not rewritten in pass 1 (#{exp&.file}##{exp&.line}, context: #{@context.inspect})"
        end
        if VOID_FUNCTIONS.include?(fn)
          decl "#{fn}(#{args.map { |a| p(a) }.join(', ')});"
          ''
        else
          result_name = name || temp(fn)
          decl "#{type} *#{result_name} = #{fn}(#{args.map { |a| p(a) }.join(', ')});"
          result_name
        end
      end

      def process_tern(exp)
        (_, condition, true_body, false_body) = exp
        result_name = temp('if_result')
        decl "NatObject *#{result_name} = nat_truthy(#{p condition}) ? #{true_body}(env, self) : #{false_body}(env, self);"
        result_name
      end

      def process_with_self(exp)
        (_, new_self, body) = exp
        self_was = temp('self_was')
        decl "NatObject *#{self_was} = self;"
        decl "self = #{p new_self};"
        result = temp('sclass_result')
        decl "NatObject *#{result} = #{p body};"
        decl "self = #{self_was};"
        result
      end

      def temp(name)
        @var_num += 1
        "#{var_prefix}#{name}#{@var_num}"
      end

      def top(code = nil)
        if code
          Array(code).each do |line|
            @top << line
          end
        else
          @top.join("\n")
        end
      end

      def decl(code = nil)
        if code
          Array(code).each do |line|
            @decl << line
          end
        else
          @decl.join("\n")
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
        decl "env->file = heap_string(#{exp.file.inspect}); env->line = #{exp.line || 0};" if exp.file
      end
    end
  end
end
