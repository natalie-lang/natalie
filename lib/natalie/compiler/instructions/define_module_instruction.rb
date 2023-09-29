require_relative './base_instruction'

module Natalie
  class Compiler
    class DefineModuleInstruction < BaseInstruction
      def initialize(name:)
        @name = name
      end

      def has_body?
        true
      end

      attr_reader :name

      def to_s
        "define_module #{@name}"
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :define_module)
        fn = transform.temp("module_#{@name}")
        transform.with_new_scope(body) do |t|
          fn_code = []
          fn_code << "Value #{fn}(Env *env, Value self) {"
          fn_code << t.transform('return')
          fn_code << '}'
          transform.top(fn_code)
        end
        mod = transform.temp('module')
        namespace = transform.pop
        code = []
        code << "auto #{mod} = #{namespace}->const_find_with_autoload(env, self, #{transform.intern(@name)}, Object::ConstLookupSearchMode::Strict, Object::ConstLookupFailureMode::Null)"
        code << "if (!#{mod}) {"
        code << "  #{mod} = new ModuleObject(#{@name.to_s.inspect})"
        code << "  #{namespace}->const_set(#{transform.intern(@name)}, #{mod})"
        code << "}"
        code << "#{mod}->as_module()->eval_body(env, #{fn})"
        transform.exec_and_push(:result_of_define_module, code)
      end

      def execute(vm)
        namespace = vm.pop
        namespace = namespace.class unless namespace.respond_to?(:const_set)
        if namespace.constants.include?(@name)
          mod = namespace.const_get(@name)
        else
          mod = Module.new
          namespace.const_set(@name, mod)
        end
        vm.method_visibility = :public
        vm.with_self(mod) { vm.run }
        :no_halt
      end
    end
  end
end
