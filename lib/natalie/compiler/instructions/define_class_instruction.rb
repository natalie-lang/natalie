require_relative './base_instruction'

module Natalie
  class Compiler
    class DefineClassInstruction < BaseInstruction
      def initialize(name:, is_private:, file:, line:)
        super()
        @name = name.to_sym
        @is_private = is_private

        # source location info
        @file = file
        @line = line
      end

      def has_body?
        true
      end

      attr_reader :name, :file, :line

      def private?
        @is_private
      end

      def to_s
        s = "define_class #{@name}"
        s << ' (private)' if @is_private
        s
      end

      def generate(transform)
        transform.set_file(@file)
        transform.set_line(@line)

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
        search_mode = private? ? 'StrictPrivate' : 'Strict'

        code = []
        code << "auto #{klass} = #{namespace}->const_find_with_autoload(env, self, " \
                "#{transform.intern(@name)}, Object::ConstLookupSearchMode::#{search_mode}, " \
                'Object::ConstLookupFailureMode::Null)'
        code << "if (#{klass}) {"
        code << "  if (!#{klass}->is_module()) {"
        code << "    env->raise(\"TypeError\", \"#{@name} is not a class\");"
        code << '  }'
        code << "} else {"
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

      def serialize(rodata)
        position = rodata.add(@name.to_s)
        [
          instruction_number,
          position,
          @is_private ? 1 : 0,
        ].pack('CwC')
      end

      def self.deserialize(io, rodata)
        position = io.read_ber_integer
        name = rodata.get(position, convert: :to_sym)
        is_private = io.getbool
        new(
          name:,
          is_private:,
          file: '', # FIXME
          line: 0 # FIXME
        )
      end
    end
  end
end
