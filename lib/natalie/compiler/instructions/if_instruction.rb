require_relative './base_instruction'

module Natalie
  class Compiler
    class IfInstruction < BaseInstruction
      def to_s
        'if'
      end

      def has_body?
        true
      end

      def generate_llvm(transform)
        true_body = transform.fetch_block_of_instructions(until_instruction: ElseInstruction, expected_label: :if)
        false_body = transform.fetch_block_of_instructions(expected_label: :if)
        condition = transform.truthy(transform.pop)
        baseline = transform.stack.dup
        results = nil
        yes, no, done = %w[if_true if_false if_end].map { |s| transform.name(s) }
        transform.emit "br i1 #{condition}, label %#{yes}, label %#{no}"
        [[yes, true_body], [no, false_body]].each do |label, body|
          transform.emit "#{label}:"
          transform.stack.replace(baseline)
          transform.transform(body)
          results ||= transform.stack.map { transform.slot('if_result') }
          raise CompileError, 'LLVM: unbalanced if branch' unless transform.stack.size == results.size
          transform.stack.zip(results).each { |value, slot| transform.emit "store volatile i64 #{value}, ptr #{slot}" }
          transform.emit "br label %#{done}"
        end
        transform.emit "#{done}:"
        transform.stack.replace(results.map { |slot| transform.value("load volatile i64, ptr #{slot}") })
      end
    end
  end
end
