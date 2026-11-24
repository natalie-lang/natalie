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

      def generate_llvm(transform)
        fn = transform.loaded_file(@filename)
        transform.push_call(
          "call i64 @nat_llvm_load(ptr %env, ptr #{transform.string(@filename)}, ptr #{fn}, i32 #{@require_once ? 1 : 0}, i64 #{transform.file_return_point(@filename, loaded: true)})",
        )
      end

      private

      def build_fn_name(loaded_file)
        suffix = loaded_file.relative_path.sub(/^[^a-zA-Z_]/, '_').gsub(/[^a-zA-Z0-9_]/, '_')
        "load_file_fn_#{suffix}"
      end
    end
  end
end
