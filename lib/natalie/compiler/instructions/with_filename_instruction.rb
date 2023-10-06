require_relative './base_instruction'

module Natalie
  class Compiler
    class WithFilenameInstruction < BaseInstruction
      def initialize(filename)
        @filename = filename
      end

      attr_reader :filename

      def has_body?
        true
      end

      def to_s
        "with_filename #{@filename}"
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :with_filename)
        fn = transform.loaded_files[@filename]
        unless fn
          fn = transform.temp('with_filename_fn')
          transform.loaded_files[@filename] = fn
          transform.with_new_scope(body) do |t|
            fn_code = []
            fn_code << "Value #{fn}(Env *env, Value self) {"
            fn_code << t.transform('return')
            fn_code << '}'
            transform.top(fn_code)
          end
        end
        transform.exec_and_push(
          :result_of_with_filename,
          "#{fn}(env, GlobalEnv::the()->main_obj())"
        )
      end

      def execute(vm)
        vm.with_self(vm.main) { vm.run }
        :no_halt
      end
    end
  end
end
