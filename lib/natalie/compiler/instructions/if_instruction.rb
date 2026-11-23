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
        result = transform.slot('if_result')
        yes, no, done = %w[if_true if_false if_end].map { |s| transform.name(s) }
        transform.emit "br i1 #{condition}, label %#{yes}, label %#{no}"
        [[yes, true_body], [no, false_body]].each do |label, body|
          transform.emit "#{label}:"
          transform.stack.replace(baseline)
          transform.transform(body)
          raise CompileError, 'LLVM: unbalanced if branch' unless transform.stack.size == baseline.size + 1
          transform.emit "store volatile i64 #{transform.pop}, ptr #{result}"
          transform.emit "br label %#{done}"
        end
        transform.emit "#{done}:"
        transform.stack.replace(baseline)
        transform.push(transform.value("load volatile i64, ptr #{result}"))
      end

      def generate(transform)
        true_body = transform.fetch_block_of_instructions(until_instruction: ElseInstruction, expected_label: :if)
        false_body = transform.fetch_block_of_instructions(expected_label: :if)
        condition = transform.pop
        result = transform.temp('if_result')
        code = []

        # hoisted variables need to be set to nil here
        (@env[:hoisted_vars] || {}).each do |_, var|
          code << "Value #{var.fetch(:name)} = Value::nil()"
          var[:declared] = true
        end

        code << "Value #{result}"

        transform.normalize_stack do
          code << "if (#{condition}.is_truthy()) {"
          if_size = transform.with_same_scope(true_body) { |t| code << t.transform("#{result} =") }

          code << '} else {'
          else_size = transform.with_same_scope(false_body) { |t| code << t.transform("#{result} =") }
          code << '}'

          if if_size != else_size
            puts 'code:'
            puts code
            puts
            puts 'true body:'
            puts true_body
            puts
            puts 'false body:'
            puts false_body
            raise "IfInstruction branch sizes are uneven! #{if_size} != #{else_size}\n" \
                    'This is a bug in the compiler. Please report it.'
          end
        end

        transform.exec(code)
        transform.push(result)
      end
    end
  end
end
