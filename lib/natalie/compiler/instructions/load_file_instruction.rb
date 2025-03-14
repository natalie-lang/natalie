require_relative './base_instruction'

module Natalie
  class Compiler
    class LoadFileInstruction < BaseInstruction
      def initialize(filename, require_once:)
        super()
        @filename = filename
        @require_once = require_once
      end

      attr_reader :filename

      def to_s
        s = "load_file #{@filename}"
        s << ' (require_once)' if @require_once
        s
      end

      def generate(transform)
        fn = transform.compiled_files[@filename]
        filename_sym = transform.intern(@filename)
        unless fn
          fn = transform.temp('load_file_fn')
          transform.compiled_files[@filename] = fn
          loaded_file = transform.required_ruby_file(@filename)
          transform.top("Value #{fn}(Env *, Value, bool);")
          transform.with_new_scope(loaded_file.instructions) do |t|
            fn_code = []
            fn_code << "Value #{fn}(Env *env, Value self, bool require_once) {"
            fn_code << "if (require_once && #{transform.has_file(filename_sym)}) {"
            fn_code << '  return Value::False();'
            fn_code << '}'
            fn_code << "#{transform.add_file(filename_sym)};"
            fn_code << t.transform
            fn_code << 'return Value::True();'
            fn_code << '}'
            transform.top(fn_code)
          end
        end
        transform.exec_and_push(
          :result_of_load_file,
          "#{fn}(env, GlobalEnv::the()->main_obj(), #{@require_once})"
        )
      end

      def execute(vm)
        vm.with_self(vm.main) { vm.run }
        :no_halt
      end
    end
  end
end
