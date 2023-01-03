module Natalie
  class Compiler
    class CaseIn
      def initialize(pass, exp)
        @pass = pass
        @exp = exp.dup
        _, @value, *@clauses, @else_block = exp
        @dig = []
      end

      attr_reader :instructions

      def transform(used:)
        @instructions = []

        # push a hash on the stack to store our state
        # { value: value, match_found: false, result: nil, vars: {} }
        @instructions += [
          PushSymbolInstruction.new(:value),
          @pass.transform_expression(@value, used: true),
          PushSymbolInstruction.new(:match_found),
          PushFalseInstruction.new,
          PushSymbolInstruction.new(:result),
          PushNilInstruction.new,
          PushSymbolInstruction.new(:vars),
          CreateHashInstruction.new(count: 0, bare: false),
          CreateHashInstruction.new(count: 4, bare: false),
        ]

        @clauses.each do |clause|
          if clause.sexp_type != :in
            raise "Unexpected case clause: #{clause.sexp_type}"
          end

          @instructions += transform_clause(clause)
        end

        @instructions += [
          get_state(:match_found),        # [..., state, bool]
          IfInstruction.new,
          get_state(:result),             # [..., state, result]
          ElseInstruction.new(:if),
          PushNilInstruction.new,         # [..., state, nil]
          EndInstruction.new(:if),
          SwapInstruction.new,            # [..., result, state]
          PopInstruction.new,             # [..., result]
        ]

        @instructions << PopInstruction.new unless used
        @instructions
      end

      private

      def transform_clause(exp)
        _, pattern, body = exp

        [
          get_state(:match_found),
          NotInstruction.new,
          IfInstruction.new,

          transform_pattern(pattern) do
            [
              @pass.transform_expression(body, used: true),
              set_state(:result),
              set_state(:match_found, value_instruction: PushTrueInstruction.new),
            ]
          end,

          ElseInstruction.new(:if),
          EndInstruction.new(:if),
        ]
      end

      def transform_pattern(pattern, &block)
        send("transform_#{pattern.sexp_type}", pattern, &block)
      end

      #def transform_array_pat(pattern, &block)
        #_, _, *items = pattern
        #transform_array_pat_size_check(items.size)

        #@stack << :if
        #stack_size = @stack.size
        #@instructions << IfInstruction.new

        #if items.empty?
          #@instructions << yield
        #else
          #while items.any?
            #item = items.shift
            #transform_pattern(item, &block)
          #end
        #end

        #while @stack.size > stack_size
          #@stack.pop
          #@instructions << EndInstruction.new(:if)
        #end

        #@instructions << ElseInstruction.new(:if)
      #end

      #def transform_array_pat_size_check(size)
        ## TODO: use ToArrayInstruction here?
        #@instructions += [                    # stack
          #PushArgcInstruction.new(0),         # [..., value, 0]
          #DupRelInstruction.new(1),           # [..., value, 0, value]
          #send_instruction(:size),            # [..., value, value_size]
          #PushArgcInstruction.new(1),         # [..., value, value_size, 1]
          #PushIntInstruction.new(size),       # [..., value, value_size, 1, items_size]
          #send_instruction(:==),              # [..., value, bool]
        #]
      #end

      def transform_lvar(pattern)
        _, name = pattern
        [
          push_value,
          VariableSetInstruction.new(name, local_only: true),
          yield,
        ]
      end

      def transform_lit(pattern)
        [
          push_value,
          PushArgcInstruction.new(1),
          @pass.transform_expression(pattern, used: true),
          send_instruction(:==),
          IfInstruction.new,
          yield,
          ElseInstruction.new(:if),
          EndInstruction.new(:if),
        ]
      end

      alias transform_str transform_lit

      def push_value
        return get_state(:value) if @dig.empty?

        dig_instructions = @dig.map do |index|
          case index
          when Integer
            PushIntInstruction.new(index)
          else
            raise "don't yet know how to handle index: #{index.inspect}"
          end
        end

        [
          get_state(:value),
          dig_instructions,
          CreateArrayInstruction.new(count: @dig.size),
          SwapInstruction.new,
          SendInstruction.new(
            :dig,
            args_array_on_stack: true,
            receiver_is_self: false,
            with_block: false,
            file: @exp.file,
            line: @exp.line
          )
        ]
      end

      def send_instruction(message)
        SendInstruction.new(
          message,
          receiver_is_self: false,
          with_block: false,
          file: @exp.file,
          line: @exp.line
        )
      end

      def get_state(key)
        [                                 # stack:
          PushSymbolInstruction.new(key), # [..., state, key]
          PushArgcInstruction.new(1),     # [..., state, key, 1]
          DupRelInstruction.new(2),       # [..., state, key, 1, state]
          send_instruction(:[]),          # [..., state, result]
        ]
      end

      def set_state(key, value_instruction: nil)
        if value_instruction
          [                                 # stack:
            DupInstruction.new,             # [..., state, state]
            PushSymbolInstruction.new(key), # [..., state, key]
            value_instruction,              # [..., state, key, value]
            HashPutInstruction.new,         # [..., state]
          ]
        else
          [                                 # stack:
            DupRelInstruction.new(1),       # [..., state, value, state]
            SwapInstruction.new,            # [..., state, state, value]
            PushSymbolInstruction.new(key), # [..., state, state, value, key]
            SwapInstruction.new,            # [..., state, state, key, value]
            HashPutInstruction.new,         # [..., state]
          ]
        end
      end
    end
  end
end
