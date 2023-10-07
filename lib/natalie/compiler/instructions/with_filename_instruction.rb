require_relative './base_instruction'

module Natalie
  class Compiler
    class WithFilenameInstruction < BaseInstruction
      def initialize(filename, require_once:)
        @filename = filename
        @require_once = require_once
      end

      attr_reader :filename

      def has_body?
        true
      end

      def to_s
        s = "with_filename #{@filename}"
        s << ' (require_once)' if @require_once
        s
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :with_filename)
        fn = transform.compiled_files[@filename]
        filename_sym = transform.intern(@filename)
        unless fn
          fn = transform.temp('with_filename_fn')
          transform.compiled_files[@filename] = fn
          transform.with_new_scope(body) do |t|
            fn_code = []
            fn_code << "Value #{fn}(Env *env, Value self, bool require_once) {"
            fn_code << "if (require_once && #{transform.files_var_name}.get(#{filename_sym})) return FalseObject::the();"
            fn_code << "#{transform.files_var_name}.set(#{filename_sym});"
            fn_code << t.transform
            fn_code << 'return TrueObject::the();'
            fn_code << '}'
            transform.top(fn_code)
          end
        end
        transform.exec_and_push(
          :result_of_with_filename,
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
