# frozen_string_literal: true

module Natalie
  class Compiler
    module Transformers
      class MatchRequiredNode
        attr_reader :compiler

        def initialize(compiler)
          @compiler = compiler
        end

        def call(node)
          case node.pattern.type
          when :array_pattern_node
            transform_array_pattern_node(node.pattern, node.value)
          when :local_variable_target_node
            transform_local_variable_target_node(node.pattern, node.value)
          else
            transform_eqeqeq_check(node.pattern, node.value)
          end
        end

        private

        def transform_array_pattern_node(node, _value)
          raise SyntaxError, "Pattern not yet supported: #{node.inspect}"
        end

        def transform_eqeqeq_check(node, value)
          [
            compiler.transform_expression(value, used: true),
            DupInstruction.new, # Required for the error message
            compiler.transform_expression(node, used: true),
            SwapInstruction.new,
            PushArgcInstruction.new(1),
            SendInstruction.new(
              :===,
              args_array_on_stack: false,
              receiver_is_self: false,
              with_block: false,
              has_keyword_hash: false,
              file: compiler.file.path,
              line: node.location.start_line,
            ),
            IfInstruction.new,
            PopInstruction.new,
            ElseInstruction.new(:if),
            # Comments are for stack of `1 => String`
            DupInstruction.new, # [1, 1]
            PushStringInstruction.new(''), # [1, 1, '']
            SwapInstruction.new, # [1, '', 1']
            StringAppendInstruction.new, # [1, '1'],
            PushStringInstruction.new(': '), # [1, '1', ': ']
            StringAppendInstruction.new, # [1, '1: ']
            compiler.transform_expression(node, used: true), # [1, '1: ', String]
            StringAppendInstruction.new, # [1, '1: String']
            PushStringInstruction.new(' === '), # [1, '1: String', ' === ']
            StringAppendInstruction.new, # [1, '1: String === ']
            SwapInstruction.new, # ['1: String === ', 1]
            StringAppendInstruction.new, # ['1: String === 1']
            PushStringInstruction.new(' does not return true'),
            StringAppendInstruction.new,
            PushSelfInstruction.new, # [msg, self],
            SwapInstruction.new, # [self, msg]
            PushSelfInstruction.new, # [self, msg, self]
            ConstFindInstruction.new(:NoMatchingPatternError, strict: false), # [self, msg, NoMatchingPatternError]
            SwapInstruction.new, # [self, NoMatchingPatternError, msg]
            PushArgcInstruction.new(2),
            SendInstruction.new(
              :raise,
              args_array_on_stack: false,
              receiver_is_self: true,
              with_block: false,
              has_keyword_hash: false,
              file: compiler.file.path,
              line: node.location.start_line,
            ),
            EndInstruction.new(:if),
            PopInstruction.new,
          ]
        end

        def transform_local_variable_target_node(node, value)
          # Transform `expr => var` into `var = ->(res) { res }.call(expr)`
          # For now we create the instructions directly, but we should look into generating Ruby
          [
            DefineBlockInstruction.new(arity: 1),
            CheckArgsInstruction.new(positional: 1, keywords: []),
            PushArgInstruction.new(0),
            VariableSetInstruction.new(:result, local_only: true),
            VariableGetInstruction.new(:result),
            EndInstruction.new(:define_block),
            CreateLambdaInstruction.new(file: compiler.file.path, line: node.location.start_line),
            compiler.transform_expression(value, used: true),
            PushArgcInstruction.new(1),
            SendInstruction.new(
              :call,
              args_array_on_stack: false,
              receiver_is_self: false,
              with_block: false,
              has_keyword_hash: false,
              file: compiler.file.path,
              line: node.location.start_line,
            ),
            VariableSetInstruction.new(node.name),
          ]
        end
      end
    end
  end
end
