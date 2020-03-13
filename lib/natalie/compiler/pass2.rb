module Natalie
  class Compiler
    # Compute captured vs local-only variables and write pseudo C for variable get and set
    class Pass2 < SexpProcessor
      def initialize(compiler_context)
        super()
        self.default_method = :process_sexp
        self.warn_on_default = false
        self.require_empty = false
        self.strict = true
        @compiler_context = compiler_context
        @env = {
          # pre-existing vars from the REPL are passed in here
          vars: @compiler_context[:vars]
        }
      end

      def go(ast)
        process_and_build_vars(ast).tap do
          # the variable info is needed by the REPL
          @compiler_context[:vars] = @env[:vars]
        end
      end

      def process_and_build_vars(exp, is_block: false)
        body = process_sexp(exp)
        var_count = @env[:vars].values.count { |v| v[:captured] }
        to_declare = @env[:vars].values.select { |v| !v[:captured] }
        exp.new(:block,
          is_block || repl? ? s(:block) : s(:var_alloc, var_count),
          *to_declare.map { |v| s(:declare, "#{@compiler_context[:var_prefix]}#{v[:name]}#{v[:var_num]}", s(:nil)) },
          body)
      end

      def process_block_fn(exp)
        (sexp_type, name, body) = exp
        is_block = %i[block_fn begin_fn].include?(sexp_type)
        @env = { parent: @env, vars: {}, block: is_block }
        result = process_and_build_vars(body, is_block: is_block)
        @env = @env[:parent]
        exp.new(sexp_type, name, result)
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

      # when using a REPL, variables are mistaken for method calls
      def process_nat_send(exp)
        return process_sexp(exp) unless repl?
        (_, receiver, name, *args) = exp
        return process_sexp(exp) unless receiver == :self && args.last == 'NULL'
        if find_var(name.to_s)
          process_nat_var_get(exp.new(:nat_var_get, :env, s(:s, name)))
        else
          process_sexp(exp)
        end
      end

      def process_nat_var_declare(exp)
        (_, _, name) = exp
        raise "bad name: #{name.inspect}" unless name.is_a?(Sexp) && name.sexp_type == :s
        name = name.last.to_s
        (env_name, var) = find_var(name)
        if var
          exp.new(:nat_var_get, env_name, var)
        else
          (env_name, var) = declare_var(name)
          exp.new(:nat_var_set, env_name, var, s(:nil))
        end
      end

      def process_nat_var_get(exp)
        (_, _, name) = exp
        raise "bad name: #{name.inspect}" unless name.is_a?(Sexp) && name.sexp_type == :s
        name = name.last.to_s
        (env_name, var) = find_var(name)
        unless var
          puts "Compile Error: undefined local variable `#{name}'"
          puts "#{exp.file}##{exp.line}"
          exit 1
        end
        exp.new(:nat_var_get, env_name, var)
      end

      def process_nat_var_set(exp)
        (_, _, name, value) = exp
        raise "bad name: #{name.inspect}" unless name.is_a?(Sexp) && name.sexp_type == :s
        name = name.last.to_s
        bare_name = name.sub(/^[\*\&]/, '')
        (env_name, var) = find_var(bare_name) || declare_var(bare_name)
        if value
          exp.new(:nat_var_set, env_name, var, process_arg(value))
        else
          exp.new(:nat_var_set, env_name, var)
        end
      end

      def process_sexp(exp)
        (name, *args) = exp
        exp.new(name, *args.map { |a| process_arg(a) })
      end

      private

      def find_var(name, env_name: 'env', env: @env, capture: false)
        if (var = env[:vars][name])
          var[:captured] = true if capture
          [env_name, var]
        elsif env[:parent] && env[:block]
          find_var(name, env_name: "#{env_name}->outer", env: env[:parent], capture: true)
        end
      end

      def declare_var(name)
        var_num = @compiler_context[:var_num] += 1
        var = @env[:vars][name] = { name: name, index: @env[:vars].size, var_num: var_num }
        var[:captured] = true if repl?
        ['env', var]
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

      def repl?
        !!@compiler_context[:repl]
      end
    end
  end
end
