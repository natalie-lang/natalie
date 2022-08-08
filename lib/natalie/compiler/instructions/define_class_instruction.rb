require_relative './base_instruction'

module Natalie
  class Compiler
    class DefineClassInstruction < BaseInstruction
      def initialize(name:)
        super()
        @name = name.to_sym
      end

      def has_body?
        true
      end

      attr_reader :name

      def to_s
        "define_class #{@name}"
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :define_class)
        fn = transform.temp("class_#{@name}")
        transform.with_new_scope(body) do |t|
          fn_code = []
          fn_code << "Value #{fn}(Env *env, Value self) {"
          fn_code << t.transform('return')
          fn_code << '}'
          transform.top(fn_code)
        end
        klass = transform.temp('class')
        namespace = transform.pop
        superclass = transform.pop
        code = []
        code << "auto #{klass} = #{namespace}->const_get(#{transform.intern(@name)})"
        code << "if (!#{klass}) {"
        code << "  #{klass} = #{superclass}->subclass(env, #{@name.to_s.inspect})"
        code << "  #{namespace}->const_set(#{transform.intern(@name)}, #{klass})"
        code << '}'
        code << "#{klass}->as_class()->eval_body(env, #{fn})"
        transform.exec_and_push(:result_of_define_class, code)
      end

      def execute(vm)
        namespace = vm.pop
        namespace = namespace.class unless namespace.respond_to?(:const_set)
        superclass = vm.pop
        if namespace.constants.include?(@name)
          klass = namespace.const_get(@name)
        else
          klass = Class.new(superclass)
          namespace.const_set(@name, klass)
        end
        vm.method_visibility = :public
        vm.with_self(klass) { vm.run }
        :no_halt
      end
    end
  end
end
