require_relative './base_instruction'

module Natalie
  class Compiler
    class OnceInstruction < BaseInstruction
      def to_s
        'once'
      end

      def has_body?
        true
      end

      def generate_llvm(transform)
        body = transform.fetch_block_of_instructions(expected_label: :once)
        baseline = transform.stack.dup
        fn, size = transform.region(body, baseline)
        frame = transform.argument_array(baseline)
        transform.stack.replace(baseline.take(size))
        cache = transform.literal_cache([:once, self])
        transform.push_call(
          "call i64 @nat_llvm_once(ptr %env, i64 %self, ptr #{transform.function_args}, ptr #{transform.function_block}, ptr #{cache}, ptr #{fn}, ptr #{frame})",
        )
      end

      def generate(transform)
        body = transform.fetch_block_of_instructions(expected_label: :once)
        cache = transform.temp('once_cache')
        guard = transform.temp('once_guard')
        result = transform.temp('once_result')
        code = []
        (@env[:hoisted_vars] || {}).each do |_, var|
          code << "Value #{var.fetch(:name)} = Value::nil()"
          var[:declared] = true
        end
        code << "static OnceCache *#{cache} = nullptr"
        code << "Value #{result}"
        code << '{'
        code << "OnceCache::Guard #{guard}(env, &#{cache})"
        code << "if (auto cached = #{guard}.value()) {"
        code << "#{result} = cached.value()"
        code << '} else {'
        transform.normalize_stack { transform.with_same_scope(body) { |t| code << t.transform("#{result} =") } }
        code << "#{guard}.set(#{result})"
        code << '}'
        code << '}'
        transform.exec(code)
        transform.push(result)
      end
    end
  end
end
