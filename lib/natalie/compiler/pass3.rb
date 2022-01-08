require_relative './base_pass'

module Natalie
  class Compiler
    # Replace non-captured variable references with local C++ variables.
    class Pass3 < BasePass
      def initialize(compiler_context)
        super
        @env = { vars: {} }
      end

      def process_var_get(exp)
        _, env, var = exp
        if var[:captured]
          exp.new(:var_get, env, s(:s, var[:name]), var[:index])
        else
          exp.new(:l, "#{@compiler_context[:var_prefix]}#{var[:name]}#{var[:var_num]}")
        end
      end

      def process_var_set(exp)
        _, env, var, allocate, value = exp
        if var[:captured]
          exp.new(:var_set, env, s(:s, var[:name]), var[:index], allocate, process(value))
        else
          exp.new(:set, "#{@compiler_context[:var_prefix]}#{var[:name]}#{var[:var_num]}", process(value))
        end
      end
    end
  end
end
