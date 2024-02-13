require_relative './base_instruction'

module Natalie
  class Compiler
    class GlobalVariableGetInstruction < BaseInstruction
      def initialize(name)
        @name = name.to_sym
      end

      attr_reader :name

      def to_s
        "global_variable_get #{@name}"
      end

      def generate(transform)
        case @name
        when :$!
          transform.exec_and_push(:exception, "env->exception_object()")
        when :$@
          transform.exec_and_push(:backtrace, "env->exception_object()->is_exception() ? static_cast<Value>(env->exception_object()->as_exception()->backtrace(env)) : static_cast<Value>(NilObject::the())")
        when :$~
          transform.exec_and_push(:last_match, "env->last_match()")
        when :$", :$LOADED_FEATURES
          transform.exec_and_push(:loaded_features, "GlobalEnv::the()->loaded_features(files)")
        else
          transform.exec_and_push(:gvar, "env->global_get(#{transform.intern(@name)})")
        end
      end

      def execute(vm)
        case @name
        when :$!
          vm.push($!)
        when :$~
          vm.push($~)
        else
          vm.push(vm.global_variables[@name])
        end
      end
    end
  end
end
