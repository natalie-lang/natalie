module Natalie
  class Compiler
    # Convert named variables to variable indices
    class Pass2 < SexpProcessor
      def initialize(compiler_context)
        super()
        self.default_method = :process_sexp
        self.warn_on_default = false
        self.require_empty = false
        self.strict = true
        @compiler_context = compiler_context
        @env = { vars: {} }
      end

      def go(ast)
        process(ast)
      end

      def process_block_fn(exp)
        (sexp_type, name, body) = exp
        is_block = %i[block_fn begin_fn].include?(sexp_type)
        @env = { parent: @env, vars: {}, block: is_block }
        body = process_sexp(body)
        var_count = @env[:vars].size
        @env = @env[:parent]
        if is_block
          exp.new(sexp_type, name, body)
        else
          exp.new(sexp_type,
            name,
            exp.new(:block,
              s(:var_alloc, var_count),
              body))
        end
      end
      alias process_begin_fn process_block_fn
      alias process_class_fn process_block_fn
      alias process_def_fn process_block_fn
      alias process_module_fn process_block_fn

      def process_defined(exp)
        (_, name) = exp
        if name.sexp_type == :lvar && find_var(name.last.to_s)
          exp.new(:nat_string, :env, s(:s, 'local-variable'))
        else
          exp
        end
      end

      def process_nat_var_get(exp)
        (_, _, name) = exp
        raise "bad name: #{name.inspect}" unless name.is_a?(Sexp) && name.sexp_type == :s
        name = name.last.to_s
        if @compiler_context[:built_in_constants].include?(name)
          exp.new(:built_in_const, name)
        else
          (env_name, index) = find_var(name)
          unless index
            puts "Compile Error: undefined local variable `#{name}'"
            puts "#{exp.file}##{exp.line}"
            exit 1
          end
          exp.new(:nat_var_get2, env_name, s(:s, name), index)
        end
      end

      def process_nat_var_get_or_set(exp)
        (_, _, name, body) = exp
        raise "bad name: #{name.inspect}" unless name.is_a?(Sexp) && name.sexp_type == :s
        name = name.last.to_s
        bare_name = name.sub(/^[\*\&]/, '')
        if @compiler_context[:built_in_constants].include?(name)
          exp.new(:built_in_const, name)
        else
          (env_name, index) = find_var(bare_name)
          if index
            exp.new(:nat_var_get2, env_name, s(:s, name), index)
          else
            process_nat_var_set(exp.new(:nat_var_set, :env, s(:s, name), body))
          end
        end
      end

      def process_nat_var_set(exp)
        (_, _, name, value) = exp
        raise "bad name: #{name.inspect}" unless name.is_a?(Sexp) && name.sexp_type == :s
        name = name.last.to_s
        bare_name = name.sub(/^[\*\&]/, '')
        (env_name, index) = find_var(bare_name)
        unless index
          index = @env[:vars][bare_name] = @env[:vars].size
          env_name = 'env'
        end
        if value
          exp.new(:nat_var_set2, env_name, s(:s, name), index, process_arg(value))
        else
          exp.new(:nat_var_set2, env_name, s(:s, name), index)
        end
      end

      def process_sexp(exp)
        (name, *args) = exp
        exp.new(name, *args.map { |a| process_arg(a) })
      end

      private

      def constant?(name)
        name.start_with?(/[A-Z]/)
      end

      def find_var(name, env_name: 'env', env: @env)
        if (index = env[:vars][name])
          [env_name, index]
        elsif env[:parent] && (env[:block] || constant?(name))
          find_var(name, env_name: "#{env_name}->outer", env: env[:parent])
        end
      end

      def process_arg(exp)
        case exp
        when Sexp
          process(exp)
        when String, Symbol, Integer, nil
          exp
        else
          raise "unknown node type: #{exp.inspect}"
        end
      end

    end
  end
end
