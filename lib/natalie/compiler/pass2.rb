require_relative './base_pass'

module Natalie
  class Compiler
    # Compute captured vs local-only variables.
    class Pass2 < BasePass
      def initialize(compiler_context)
        super
        @env = {
          # pre-existing vars from the REPL are passed in here
          vars: @compiler_context[:vars],
        }
      end

      def go(ast)
        process_and_build_vars(ast)
      end

      def process_and_build_vars(exp)
        body = process_sexp(exp)
        var_count = @env[:vars].size # FIXME: this is overkill -- there are variables not captured in this count, i.e. "holes" in the array :-(
        to_declare = @env[:vars].values.select { |v| !v[:captured] }
        exp.new(
          :block,
          repl? ? s(:block) : s(:var_alloc, var_count),
          *to_declare.map { |v| s(:declare, "#{@compiler_context[:var_prefix]}#{v[:name]}#{v[:var_num]}", s(:nil)) },
          body,
        )
      end

      def process_block_fn(exp)
        sexp_type, name, body = exp
        is_block = %i[block_fn begin_fn].include?(sexp_type)
        should_hoist = sexp_type == :begin_fn
        @env = { parent: @env, vars: {}, block: is_block, hoist: should_hoist }
        result = process_and_build_vars(body)
        @env = @env[:parent]
        exp.new(sexp_type, name, result)
      end
      alias process_begin_fn process_block_fn
      alias process_class_fn process_block_fn
      alias process_def_fn process_block_fn
      alias process_module_fn process_block_fn

      def process_defined(exp)
        _, name = exp
        if name.sexp_type == :lvar && find_var(name.last.to_s)
          exp.new(:new, :StringObject, s(:s, 'local-variable'))
        elsif %i[send public_send].include?(name.sexp_type)
          sexp, obj, sym, (_, *args), block = exp
          exp.new(sexp, process(obj), sym, s(:args, *args.map { |a| process(a) }), block)
        else
          exp
        end
      end

      def process_arg_set(exp)
        _, _, name, value = exp
        raise "bad name: #{name.inspect}" unless name.is_a?(Sexp) && name.sexp_type == :s
        name = name.last.to_s
        bare_name = name.sub(/^[\*\&]/, '')
        env_name, var = declare_var(bare_name)
        value = value ? process_atom(value) : s(:nil)
        exp.new(:var_set, env_name, var, repl?, value)
      end

      def process_public_send(exp)
        process_send(exp)
      end

      # when using a REPL, variables are mistaken for method calls
      def process_send(exp)
        return process_sexp(exp) unless repl?
        _, receiver, (_, name), *args = exp
        return process_sexp(exp) unless receiver == :self && args.last == 'nullptr'
        find_var(name.to_s) ? process_var_get(exp.new(:var_get, :env, s(:s, name))) : process_sexp(exp)
      end

      def process_var_declare(exp)
        _, _, name = exp
        raise "bad name: #{name.inspect}" unless name.is_a?(Sexp) && name.sexp_type == :s
        name = name.last.to_s
        env_name, var = find_var(name)
        if var
          exp.new(:var_get, env_name, var)
        else
          env_name, var = declare_var(name)
          exp.new(:var_set, env_name, var, repl?, s(:nil))
        end
      end

      def process_var_get(exp)
        _, _, name = exp
        raise "bad name: #{name.inspect}" unless name.is_a?(Sexp) && name.sexp_type == :s
        name = name.last.to_s
        env_name, var = find_var(name)
        unless var
          puts "Compile Error: undefined local variable `#{name}'"
          puts "#{exp.file}##{exp.line}"
          exit 1
        end
        exp.new(:var_get, env_name, var)
      end

      def process_var_set(exp)
        _, _, name, value = exp
        raise "bad name: #{name.inspect}" unless name.is_a?(Sexp) && name.sexp_type == :s
        name = name.last.to_s
        bare_name = name.sub(/^[\*\&]/, '')
        env_name, var = find_var(bare_name) || declare_var(bare_name)
        value = value ? process_atom(value) : s(:nil)
        exp.new(:var_set, env_name, var, repl?, value)
      end

      private

      def find_var(name, env_name: 'env', env: @env, capture: false)
        if (var = env[:vars][name])
          var[:captured] = true if capture
          [env_name, var]
        elsif env[:parent] && env[:block]
          find_var(name, env_name: "#{env_name}->outer()", env: env[:parent], capture: true)
        end
      end

      def declare_var(name)
        var_num = @compiler_context[:var_num] += 1
        if @env[:hoist]
          env = @env[:parent]
          env_name = 'env->outer()'
          while env[:hoist]
            env = env[:parent]
            env_name = "#{env_name}->outer()"
          end
          var = env[:vars][name] = { name: name, index: env[:vars].size, var_num: var_num, captured: true }
          [env_name, var]
        else
          var = @env[:vars][name] = { name: name, index: @env[:vars].size, var_num: var_num }
          var[:captured] = true if repl?
          ['env', var]
        end
      end

      def repl?
        !!@compiler_context[:repl]
      end
    end
  end
end
