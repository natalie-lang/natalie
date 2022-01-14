require_relative './base_instruction'

module Natalie
  class Compiler2
    class DefineClassInstruction < BaseInstruction
      def initialize(name:, superclass:)
        @name = name
        @superclass = superclass
      end

      def has_body?
        true
      end

      attr_reader :name, :superclass

      def to_s
        s = "define_class #{@name}"
        s << " from #{@superclass}" if @superclass
        s
      end

      def to_cpp(transform)
        body = transform.fetch_block_of_instructions(expected_label: :define_class)
        fn = transform.temp("class_#{@name}")
        transform.with_new_scope(body) do |t|
          body = []
          body << "Value #{fn}(Env *env, Value self) {"
          body << t.transform('return')
          body << '}'
          transform.top(body)
        end
        klass = transform.temp('class')
        raise 'FIXME: implement superclass' if @superclass
        superclass = 'GlobalEnv::the()->Object()'
        code = []
        code << "auto #{klass} = #{superclass}->subclass(env, #{@name.to_s.inspect})"
        code << "self->const_set(#{@name.to_s.inspect}_s, #{klass})"
        code << "#{klass}->eval_body(env, #{fn})"
        transform.push(code)
        nil
      end

      def execute(vm)
        raise 'FIXME: implement superclass' if @superclass
        klass = Class.new
        Object.const_set(@name, klass)
        vm.with_self(klass) do
          vm.run
        end
      end
    end
  end
end
