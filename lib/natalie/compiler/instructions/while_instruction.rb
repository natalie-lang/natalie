require_relative './base_instruction'

module Natalie
  class Compiler
    class WhileInstruction < BaseInstruction
      def initialize(pre:)
        @pre = pre
      end

      attr_reader :pre

      def to_s
        s = 'while'
        s << ' (post)' unless pre
        s
      end

      def has_body?
        true
      end

      class << self
        def reset_result_id
          @result_id = 0
        end

        def next_result_name
          @result_id ||= 0
          "while_result#{@result_id += 1}"
        end
      end

      def result_name
        @result_name ||= self.class.next_result_name
      end

      def generate_llvm(transform)
        condition = transform.fetch_block_of_instructions(until_instruction: WhileBodyInstruction)
        body = transform.fetch_block_of_instructions(expected_label: :while)
        baseline = transform.stack.dup
        points = %i[break next redo].to_h { |kind| [kind, transform.break_point([:loop, env.object_id, kind])] }
        condition_fn = body_fn = nil
        transform.with_loop(points) do
          condition_fn, = transform.region(condition, baseline)
          body_fn, = transform.region(body, baseline)
        end
        frame = transform.argument_array(baseline)
        result = transform.slot('loop_result')
        value = transform.slot('loop_value')
        test, run, check, finish, done =
          %w[while_test while_body while_check while_break while_end].map { |s| transform.name(s) }
        transform.emit "br label %#{@pre ? test : run}"
        [[test, condition_fn, check], [run, body_fn, test]].each do |label, fn, normal|
          transform.emit "#{label}:"
          status =
            transform.value(
              "call i32 @nat_llvm_loop_step(ptr %env, i64 %self, ptr #{transform.function_args}, ptr #{transform.function_block}, ptr #{fn}, ptr #{frame}, ptr #{transform.function_exception}, ptr #{value}, i64 #{points.fetch(:break)}, i64 #{points.fetch(:next)}, i64 #{points.fetch(:redo)})",
            )
          transform.emit "switch i32 #{status}, label %#{normal} [ i32 1, label %#{finish} i32 2, label %#{test} i32 3, label %#{run} ]"
        end
        transform.emit "#{check}:"
        condition_value = transform.truthy(transform.value("load volatile i64, ptr #{value}"))
        transform.emit "br i1 #{condition_value}, label %#{run}, label %#{done}"
        transform.emit "#{finish}:"
        exit_value = transform.value("load volatile i64, ptr #{value}")
        transform.emit "store volatile i64 #{exit_value}, ptr #{result}"
        transform.emit "br label %#{done}"
        transform.emit "#{done}:"
        transform.stack.replace(baseline)
        transform.push(transform.value("load volatile i64, ptr #{result}"))
      end

      def generate(transform)
        condition = transform.fetch_block_of_instructions(until_instruction: WhileBodyInstruction)
        body = transform.fetch_block_of_instructions(until_instruction: EndInstruction, expected_label: :while)
        code = []

        # hoisted variables need to be set to nil here
        (@env[:hoisted_vars] || {}).each do |_, var|
          code << "Value #{var.fetch(:name)} = Value::nil()"
          var[:declared] = true
        end

        condition_name = transform.temp('while_condition')

        transform.with_same_scope(condition) do |t|
          code << "auto #{condition_name} = [&]() {"
          code << '  if (FiberObject::current()->check_redo_flag_and_clear()) { return Value::True(); }'
          code << t.transform('return')
          code << '};'
        end

        code << "Value #{result_name} = Value::nil()"

        transform.with_same_scope(body) do |t|
          if @pre
            code << "while (#{condition_name}().is_truthy()) {"
            code << t.transform
            code << '}'
          else
            code << 'do {'
            code << t.transform
            code << "} while (#{condition_name}().is_truthy())"
          end
        end

        transform.exec(code)
        transform.push(result_name)
      end
    end
  end
end
