require_relative './args'
require_relative './arity'
require_relative './base_pass'
require_relative './const_prepper'
require_relative './multiple_assignment'
require_relative './rescue'

module Natalie
  class Compiler
    # This compiler pass transforms AST from the Parser into Intermediate
    # Representation, which we implement using Instructions.
    # You can debug this pass with the `-d p1` CLI flag.
    class Pass1 < BasePass
      def initialize(ast, compiler_context:, macro_expander:)
        super()
        @ast = ast
        @compiler_context = compiler_context
        @macro_expander = macro_expander

        # If any user code has required 'natalie/inline', then we enable
        # magical extra features. :-)
        @inline_cpp_enabled = @compiler_context[:inline_cpp_enabled]

        # We need a way to associate `retry` with the `rescue` block it
        # belongs to. Using a stack of object ids seems to work ok.
        # See the Rescue class for how it's used.
        @retry_context = []

        # We need to know if we're at the top level (left-most indent)
        # of a file, which enables certain macros.
        @depth = 0
      end

      INLINE_CPP_MACROS = %i[
        __bind_method__
        __bind_static_method__
        __call__
        __constant__
        __cxx_flags__
        __define_method__
        __function__
        __inline__
        __internal_inline_code__
        __ld_flags__
      ].freeze

      # pass used: true to leave the final result on the stack
      def transform(used: false)
        raise 'unexpected AST input' unless @ast.type == :statements_node
        transform_statements_node(@ast, used: used).flatten
      end

      def transform_expression(node, used:)
        case node
        when Sexp, ::Prism::Node
          node = expand_macros(node) if node.type == :call_node
          @depth += 1 unless node.type == :statements_node
          method = "transform_#{node.type}"
          result = send(method, node, used: used)
          @depth -= 1 unless node.type == :statements_node
          Array(result).flatten
        else
          raise "Unknown node type: #{node.inspect}"
        end
      end

      def expand_macros(node)
        @macro_expander.expand(node, depth: @depth)
      end

      def transform_body(body, used:)
        body = body.body if body.is_a?(Prism::StatementsNode)
        *body, last = body
        instructions = body.map { |exp| transform_expression(exp, used: false) }
        instructions << transform_expression(last || Prism.nil_node, used: used)
        instructions
      end

      def retry_context(id)
        @retry_context << id
        yield
      ensure
        @retry_context.pop
      end

      private

      # INDIVIDUAL PRISM NODES = = = = =
      # (in alphabetical order)

      def transform_alias_method_node(node, used:)
        instructions = [transform_expression(node.new_name, used: true)]
        instructions << DupInstruction.new if used
        instructions << transform_expression(node.old_name, used: true)
        instructions << AliasInstruction.new
      end

      def transform_and_node(node, used:)
        instructions = [
          *transform_expression(node.left, used: true),
          DupInstruction.new,
          IfInstruction.new,
          PopInstruction.new,
          *transform_expression(node.right, used: true),
          ElseInstruction.new(:if),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_arguments_node_for_callish(node)
        args = node&.arguments || []
        instructions = []

        if args.last&.sexp_type == :block_argument_node
          block_arg = args.pop
          instructions.unshift(transform_expression(block_arg.expression, used: true))
        end

        if args.size == 1 && args.first.type == :forwarding_arguments_node && !block
          instructions.unshift(PushBlockInstruction.new)
          block = true
        end

        if args.any? { |a| a.sexp_type == :splat_node }
          instructions << transform_array_with_splat(args)
          return {
            instructions: instructions,
            with_block_pass: !!block,
            args_array_on_stack: true,
            has_keyword_hash: args.last&.sexp_type == :bare_hash
          }
        end

        # special ... syntax
        if args.size == 1 && args.first.type == :forwarding_arguments_node
          instructions << PushArgsInstruction.new(
            for_block: false,
            min_count: nil,
            max_count: nil,
            spread: false,
            to_array: false,
          )
          return {
            instructions: instructions,
            with_block_pass: !!block,
            args_array_on_stack: true,
            has_keyword_hash: false,
            forward_args: true,
          }
        end

        args.each do |arg|
          instructions << transform_expression(arg, used: true)
        end

        has_keyword_hash = args.last&.sexp_type == :bare_hash

        instructions << PushArgcInstruction.new(args.size)

        {
          instructions: instructions,
          with_block_pass: !!block,
          args_array_on_stack: false,
          has_keyword_hash: has_keyword_hash,
        }
      end

      def transform_arguments_node_for_returnish(node, location:)
        case node&.arguments&.size || 0
        when 0
          nil_node = Prism.nil_node(location: location)
          transform_expression(nil_node, used: true)
        when 1
          transform_expression(node.arguments.first, used: true)
        else
          array = Prism.array_node(elements: node.arguments, location: location)
          transform_expression(array, used: true)
        end
      end

      def transform_array_node(node, used:)
        elements = node.elements
        instructions = []

        if node.elements.any? { |a| a.sexp_type == :splat_node }
          instructions += transform_array_with_splat(elements)
        else
          elements.each do |element|
            instructions << transform_expression(element, used: true)
          end
          instructions << CreateArrayInstruction.new(count: elements.size)
        end

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_back_reference_read_node(node, used:)
        return [] unless used
        case node.slice
        when '$`', "$'"
          [GlobalVariableGetInstruction.new(node.slice.to_sym)]
        when '$&'
          [PushLastMatchInstruction.new(to_s: true)]
        else
          raise "unknown back ref read node: #{node.slice}"
        end
      end

      def transform_break_node(node, used:)
        [
          transform_arguments_node_for_returnish(node.arguments, location: node.location),
          BreakInstruction.new
        ]
      end

      def transform_call_node(node, used:, with_block: false)
        receiver = node.receiver
        message = node.name
        args = node.arguments&.arguments || []

        if repl? && (new_exp = fix_repl_var_that_looks_like_call(node))
          return transform_expression(new_exp, used: used)
        end

        if is_inline_macro_call_node?(node)
          instructions = []
          if message == :__call__
            args[1..].reverse_each do |arg|
              instructions << transform_expression(arg, used: true)
            end
          end
          instructions << InlineCppInstruction.new(node)
          return instructions
        end

        if receiver.nil? && message == :lambda && args.empty? && node.block.is_a?(Prism::BlockNode)
          # NOTE: We need Kernel#lambda to behave just like the stabby
          # lambda (->) operator so we can attach the "break point" to
          # it. I realize this is a bit of a hack and if someone wants
          # to alias Kernel#lambda, then their method will create a
          # broken lambda, i.e. calling `break` won't work correctly.
          # Maybe someday we can think of a better way to handle this...
          return transform_lambda_node(node.block, used: used)
        end

        instructions = []

        # block handling
        if node.block.is_a?(Prism::BlockNode)
          with_block = true
          instructions << transform_block_node(
            node.block,
            used: true,
            is_lambda: is_lambda_call?(node)
          )
        elsif node.block.is_a?(Prism::BlockArgumentNode)
          with_block = true
          instructions << transform_expression(node.block, used: true)
        end

        # foo(...) block handling
        if args.size == 1 && args.first.type == :forwarding_arguments_node && !with_block
          instructions << PushBlockInstruction.new
          with_block = true
        end

        # block_given? works with nearest block
        if receiver.nil? && message == :block_given? && !with_block
          instructions << PushBlockInstruction.new(from_nearest_env: true)
          with_block = true
        end

        # foo.bar
        # ^
        if receiver.nil?
          instructions << PushSelfInstruction.new
        else
          instructions << transform_expression(receiver, used: true)
        end

        if node.safe_navigation?
          # wrap the SendInstruction with an effective `unless reciever.nil?`
          instructions << DupInstruction.new # duplicate receiver for IsNil below
          instructions << IsNilInstruction.new
          instructions << IfInstruction.new
          instructions << PopInstruction.new # pop duplicated receiver since it is unused
          instructions << PushNilInstruction.new
          instructions << ElseInstruction.new(:if)
        end

        call_args = transform_call_args(args, with_block: with_block, instructions: instructions)
        with_block ||= call_args.fetch(:with_block_pass)

        if node.name == :!~
          message = :=~
        end

        instructions << SendInstruction.new(
          message,
          args_array_on_stack: call_args.fetch(:args_array_on_stack),
          receiver_is_self: receiver.nil?,
          with_block: with_block,
          has_keyword_hash: call_args.fetch(:has_keyword_hash),
          forward_args: call_args[:forward_args],
          file: node.location.path,
          line: node.location.start_line,
        )

        if node.safe_navigation?
          # close out our safe navigation `if` wrapper
          instructions << EndInstruction.new(:if)
        end

        if node.name == :!~
          instructions << NotInstruction.new
        end

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_call_operator_write_node(node, used:)
        obj = node.receiver
        key_args = node.arguments&.arguments || []

        if node.write_name.to_sym == :[]=
          instructions = [
            # old_value = obj[key]
            # stack: [obj]
            transform_expression(obj, used: true),

            # stack: [obj, *keys]
            key_args.map { |arg| transform_expression(arg, used: true) },

            # stack: [obj, *keys, obj]
            DupRelInstruction.new(key_args.size),

            # stack: [obj, *keys, obj, *keys]
            key_args.each_with_index.map { |_, index| DupRelInstruction.new(index + key_args.size) },

            # old_value = obj[*keys]
            # stack: [obj, *keys, old_value]
            PushArgcInstruction.new(key_args.size),
            SendInstruction.new(
              :[],
              receiver_is_self: false,
              with_block: false,
              file: node.location.path,
              line: node.location.start_line,
            ),

            # stack: [obj, *keys, old_value, value]
            transform_expression(node.value, used: true),

            # new_value = old_value + value
            # stack: [obj, *keys, new_value]
            PushArgcInstruction.new(1),
            SendInstruction.new(
              node.operator,
              receiver_is_self: false,
              with_block: false,
              file: node.location.path,
              line: node.location.start_line,
            ),

            # obj[*keys] = new_value
            PushArgcInstruction.new(key_args.size + 1),
            SendInstruction.new(
              :[]=,
              receiver_is_self: false,
              with_block: false,
              file: node.location.path,
              line: node.location.start_line,
            ),
          ]
        else
          writer = node.write_name
          raise 'expected writer=' unless writer =~ /=$/
          reader = writer.to_s.chop
          # given: obj.foo += value
          instructions = [
            # stack: [obj]
            transform_expression(obj, used: true),

            # stack: [obj, value]
            transform_expression(node.value, used: true),

            # stack: [obj, value, obj]
            DupRelInstruction.new(1),

            # temp = obj.foo
            # stack: [obj, value, temp]
            PushArgcInstruction.new(0),
            SendInstruction.new(
              reader,
              receiver_is_self: false,
              with_block: false,
              file: node.location.path,
              line: node.location.start_line,
            ),

            # stack: [obj, temp, value]
            MoveRelInstruction.new(1),

            # result = temp + value
            # stack: [obj, new_value]
            PushArgcInstruction.new(1),
            SendInstruction.new(
              node.operator,
              receiver_is_self: false,
              with_block: false,
              file: node.location.path,
              line: node.location.start_line,
            ),

            # obj.foo = new_value
            PushArgcInstruction.new(1),
            SendInstruction.new(
              writer,
              receiver_is_self: false,
              with_block: false,
              file: node.location.path,
              line: node.location.start_line,
            ),
          ]
        end

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_call_or_write_node(node, used:)
        obj = node.receiver
        key_args = node.arguments&.arguments || []

        if node.read_name == :[]
          instructions = [
            # stack: [obj]
            transform_expression(obj, used: true),

            # stack: [obj, *keys]
            key_args.map { |arg| transform_expression(arg, used: true) },

            # stack: [obj, *keys, obj]
            DupRelInstruction.new(key_args.size),

            # stack: [obj, *keys, obj, *keys]
            key_args.each_with_index.map { |_, index| DupRelInstruction.new(index + key_args.size) },

            # old_value = obj[*keys]
            # stack: [obj, *keys, old_value]
            PushArgcInstruction.new(key_args.size),
            SendInstruction.new(
              :[],
              receiver_is_self: false,
              with_block: false,
              file: node.location.path,
              line: node.location.start_line,
            ),

            # stack: [obj, *keys, old_value, old_value]
            DupInstruction.new,

            # if old_value
            # stack: [obj, *keys, old_value]
            IfInstruction.new,

            # didn't need the extra key(s) after all :-)
            key_args.map do
              [
                SwapInstruction.new, # move value above duplicated key
                PopInstruction.new, # get rid of duplicated key
              ]
            end,

            ElseInstruction.new(:if),

            # stack: [obj, *keys]
            PopInstruction.new,

            # obj[*keys] = new_value
            # stack: [obj, *keys, new_value]
            transform_expression(node.value, used: true),
            PushArgcInstruction.new(key_args.size + 1),
            SendInstruction.new(
              :[]=,
              receiver_is_self: false,
              with_block: false,
              file: node.location.path,
              line: node.location.start_line,
            ),

            EndInstruction.new(:if),
          ]
        else
          # a.foo ||= 'bar'
          instructions = [

            # a.foo
            transform_expression(node.receiver, used: true),
            PushArgcInstruction.new(0),
            SendInstruction.new(
              node.read_name,
              receiver_is_self: false,
              with_block: false,
              file: node.location.path,
              line: node.location.start_line,
            ),

            # duplicate for use by the if truthy case
            DupInstruction.new,

            # if a.foo
            IfInstruction.new,
            # if a.foo, return duplicated value

            ElseInstruction.new(:if),

            # remove duplicated value that was falsey
            PopInstruction.new,

            # a.foo=('bar')
            transform_expression(node.receiver, used: true),
            transform_expression(node.value, used: true),
            PushArgcInstruction.new(1),
            SendInstruction.new(
              node.write_name,
              receiver_is_self: false,
              with_block: false,
              file: node.location.path,
              line: node.location.start_line,
            ),

            EndInstruction.new(:if),
          ]
        end

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_case_node(node, used:)
        return transform_expression(node.consequent, used: used) if node.conditions.empty?

        instructions = []
        if node.predicate
          instructions << transform_expression(node.predicate, used: true)
          node.conditions.each do |when_statement|
            # case a
            # when b, c, d
            # =>
            # if (b === a || c === a || d === a)
            _, options_array, *body = when_statement
            options = options_array.elements

            options.each do |option|
              # Splats are handled in the backend.
              # For C++, it's done in the is_case_equal() function.
              if option.sexp_type == :splat_node
                option = option.expression
                is_splat = true
              else
                is_splat = false
              end

              option_instructions = transform_expression(option, used: true)
              instructions << option_instructions
              instructions << CaseEqualInstruction.new(is_splat: is_splat)
              instructions << IfInstruction.new
              instructions << PushTrueInstruction.new
              instructions << ElseInstruction.new(:if)
            end
            instructions << PushFalseInstruction.new
            instructions << [EndInstruction.new(:if)] * options.length
            instructions << IfInstruction.new
            instructions << transform_body(body, used: true)
            instructions << ElseInstruction.new(:if)
          end
        else
          # glorified if-else
          node.conditions.each do |when_statement|
            # case
            # when a == b, b == c, c == d
            # =>
            # if (a == b || b == c || c == d)
            _, options, *body = when_statement

            # s(:array, option1, option2, ...)
            # =>
            # s(:or, option1, s(:or, option2, ...))
            options = options.elements
            options = options[1..].reduce(options[0]) { |prev, option| Prism.or_node(left: prev, right: option) }

            instructions << transform_expression(options, used: true)
            instructions << IfInstruction.new
            instructions << transform_body(body, used: true)
            instructions << ElseInstruction.new(:if)
          end
        end

        instructions << if node.consequent.nil?
                          PushNilInstruction.new
                        else
                          transform_expression(node.consequent, used: true)
                        end

        instructions << [EndInstruction.new(:if)] * node.conditions.length

        # The case value is never popped during comparison, so we have to pop it here.
        instructions << [SwapInstruction.new, PopInstruction.new] if node.predicate

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_class_node(node, used:)
        instructions = []
        if node.superclass
          instructions << transform_expression(node.superclass, used: true)
        else
          instructions << PushObjectClassInstruction.new
        end
        name, is_private, prep_instruction = constant_name(node.constant_path)
        instructions << prep_instruction
        instructions << DefineClassInstruction.new(
          name: name,
          is_private: is_private,
          file: node.location.path,
          line: node.location.start_line,
        )
        instructions += transform_body(node.body, used: true)
        instructions << EndInstruction.new(:define_class)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_class_variable_read_node(node, used:)
        return [] unless used
        ClassVariableGetInstruction.new(node.name)
      end

      def transform_class_variable_write_node(node, used:)
        instructions = [transform_expression(node.value, used: true), ClassVariableSetInstruction.new(node.name)]
        instructions << ClassVariableGetInstruction.new(node.name) if used
        instructions
      end

      def transform_class_variable_and_write_node(node, used:)
        instructions = [
          ClassVariableGetInstruction.new(node.name, default_to_nil: true),
          IfInstruction.new,
          transform_expression(node.value, used: true),
          ClassVariableSetInstruction.new(node.name),
          ClassVariableGetInstruction.new(node.name),
          ElseInstruction.new(:if),
          ClassVariableGetInstruction.new(node.name, default_to_nil: true),
          EndInstruction.new(:if)
        ]

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_class_variable_or_write_node(node, used:)
        instructions = [
          ClassVariableGetInstruction.new(node.name, default_to_nil: true),
          IfInstruction.new,
          ClassVariableGetInstruction.new(node.name, default_to_nil: true),
          ElseInstruction.new(:if),
          transform_expression(node.value, used: true),
          ClassVariableSetInstruction.new(node.name),
          ClassVariableGetInstruction.new(node.name),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_class_variable_operator_write_node(node, used:)
        instructions = [
          ClassVariableGetInstruction.new(node.name, default_to_nil: true),
          transform_expression(node.value, used: true),
          PushArgcInstruction.new(1),
          SendInstruction.new(node.operator, receiver_is_self: false, with_block: false, file: node.file, line: node.line),
          ClassVariableSetInstruction.new(node.name)
        ]
        instructions << ClassVariableGetInstruction.new(node.name) if used
        instructions
      end

      def transform_constant_path_node(node, used:)
        name, _is_private, prep_instruction = constant_name(node)
        # FIXME: is_private shouldn't be ignored I think
        return [] unless used
        [
          prep_instruction,
          ConstFindInstruction.new(name, strict: true),
        ]
      end

      def transform_constant_path_write_node(node, used:)
        instructions = [transform_expression(node.value, used: true)]
        instructions << DupInstruction.new if used
        name, _is_private, prep_instruction = constant_name(node.target)
        # FIXME: is_private shouldn't be ignored I think
        instructions << prep_instruction
        instructions << ConstSetInstruction.new(name)
        instructions
      end

      def transform_constant_read_node(node, used:)
        return [] unless used
        [
          PushSelfInstruction.new,
          ConstFindInstruction.new(node.name, strict: false),
        ]
      end

      def transform_constant_write_node(node, used:)
        instructions = [transform_expression(node.value, used: true)]
        instructions << DupInstruction.new if used
        instructions << PushSelfInstruction.new
        instructions << ConstSetInstruction.new(node.name)
        instructions
      end

      def transform_def_node(node, used:)
        arity = Arity.new(node.parameters, is_proc: false).arity

        instructions = []
        if node.receiver
          instructions << [
            transform_expression(node.receiver, used: true),
            SingletonClassInstruction.new,
          ]
        else
          instructions << PushSelfInstruction.new
        end

        instructions << [
          DefineMethodInstruction.new(name: node.name, arity: arity),
          transform_defn_args(node.parameters, used: true),
          transform_body([node.body], used: true),
          EndInstruction.new(:define_method),
        ]

        instructions << PushSymbolInstruction.new(node.name) if used
        instructions
      end

      def transform_defined_node(node, used:)
        return [] unless used

        body = transform_expression(node.value, used: true)
        case body.last
        when ConstFindInstruction
          type = 'constant'
        when CreateHashInstruction
          # peculiarity that hashes always return 'expression'
          type = 'expression'
          return [PushStringInstruction.new(type)]
        when GlobalVariableGetInstruction
          type = 'global-variable'
        when InstanceVariableGetInstruction
          type = 'instance-variable'
        when PushFalseInstruction
          type = 'false'
        when SendInstruction
          if body.last.with_block
            # peculiarity that send with a block always returns 'expression'
            type = 'expression'
            return [PushStringInstruction.new(type)]
          else
            type = 'method'
          end
        when PushNilInstruction
          type = 'nil'
        when PushTrueInstruction
          type = 'true'
        when VariableGetInstruction
          type = 'local-variable'
        else
          type = 'expression'
        end
        body.each_with_index do |instruction, index|
          case instruction
          when GlobalVariableGetInstruction
            body[index] = GlobalVariableDefinedInstruction.new(instruction.name)
          when InstanceVariableGetInstruction
            body[index] = InstanceVariableDefinedInstruction.new(instruction.name)
          when SendInstruction
            body[index] = instruction.to_method_defined_instruction
          end
        end
        [
          IsDefinedInstruction.new(type: type),
          body,
          EndInstruction.new(:is_defined),
        ]
      end

      def transform_false_node(_, used:)
        return [] unless used
        [PushFalseInstruction.new]
      end

      def transform_float_node(node, used:)
        return [] unless used
        [PushFloatInstruction.new(node.value)]
      end

      def transform_for_node(node, used:)
        instructions = transform_for_declare_args(node.index)
        instructions << DefineBlockInstruction.new(arity: 1)
        instructions += transform_block_args_for_for(s(:args, node.index), used: true)
        instructions += transform_expression(node.statements, used: true)
        instructions << EndInstruction.new(:define_block)
        call = Prism.call_node(receiver: node.collection, name: :each)
        instructions << transform_call_node(call, used: used, with_block: true)
        instructions
      end

      def transform_forwarding_super_node(node, used:, with_block: false)
        instructions = []

        # block handling
        if node.block.is_a?(Prism::BlockNode)
          with_block = true
          instructions << transform_block_node(
            node.block,
            used: true,
            is_lambda: is_lambda_call?(node)
          )
        elsif node.block.is_a?(Prism::BlockArgumentNode)
          with_block = true
          instructions << transform_expression(node.block, used: true)
        end

        instructions << PushSelfInstruction.new
        instructions << PushArgsInstruction.new(for_block: false, min_count: 0, max_count: 0)
        instructions << SuperInstruction.new(
          args_array_on_stack: true,
          with_block: with_block,
        )

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_global_variable_and_write_node(node, used:)
        instructions = [
          GlobalVariableGetInstruction.new(node.name),
          IfInstruction.new,
          transform_expression(node.value, used: true),
          GlobalVariableSetInstruction.new(node.name),
          ElseInstruction.new(:if),
          GlobalVariableGetInstruction.new(node.name),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_global_variable_operator_write_node(node, used:)
        instructions = [
          GlobalVariableGetInstruction.new(node.name),
          transform_expression(node.value, used: true),
          PushArgcInstruction.new(1),
          SendInstruction.new(
            node.operator,
            receiver_is_self: false,
            with_block: false,
            file: node.location.path,
            line: node.location.start_line,
          ),
        ]
        instructions << DupInstruction.new if used
        instructions << GlobalVariableSetInstruction.new(node.name)
        instructions
      end

      def transform_global_variable_or_write_node(node, used:)
        instructions = [
          GlobalVariableGetInstruction.new(node.name),
          IfInstruction.new,
          GlobalVariableGetInstruction.new(node.name),
          ElseInstruction.new(:if),
          transform_expression(node.value, used: true),
          GlobalVariableSetInstruction.new(node.name),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_global_variable_read_node(node, used:)
        return [] unless used
        GlobalVariableGetInstruction.new(node.name)
      end

      def transform_imaginary_node(node, used:)
        return [] unless used

        value = node.value.imaginary
        instruction =
          case value
          when Integer
            PushIntInstruction.new(value)
          when Float
            PushFloatInstruction.new(value)
          when Rational
            PushRationalInstruction.new(value)
          else
            raise "Unexpected imaginary value: \"#{value.inspect}\""
          end

        [
          PushIntInstruction.new(0),
          instruction,
          PushComplexInstruction.new,
        ]
      end

      def transform_instance_variable_and_write_node(node, used:)
        instructions = [
          InstanceVariableGetInstruction.new(node.name),
          IfInstruction.new,
          transform_expression(node.value, used: true),
          InstanceVariableSetInstruction.new(node.name),
          ElseInstruction.new(:if),
          InstanceVariableGetInstruction.new(node.name),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_instance_variable_operator_write_node(node, used:)
        instructions = [
          InstanceVariableGetInstruction.new(node.name),
          transform_expression(node.value, used: true),
          PushArgcInstruction.new(1),
          SendInstruction.new(
            node.operator,
            receiver_is_self: false,
            with_block: false,
            file: node.location.path,
            line: node.location.start_line,
          ),
        ]
        instructions << DupInstruction.new if used
        instructions << InstanceVariableSetInstruction.new(node.name)
        instructions
      end

      def transform_instance_variable_or_write_node(node, used:)
        instructions = [
          InstanceVariableGetInstruction.new(node.name),
          IfInstruction.new,
          InstanceVariableGetInstruction.new(node.name),
          ElseInstruction.new(:if),
          transform_expression(node.value, used: true),
          InstanceVariableSetInstruction.new(node.name),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_instance_variable_read_node(node, used:)
        return [] unless used
        InstanceVariableGetInstruction.new(node.name)
      end

      def transform_instance_variable_write_node(node, used:)
        instructions = [
          transform_expression(node.value, used: true),
        ]
        instructions << DupInstruction.new if used
        instructions << InstanceVariableSetInstruction.new(node.name)
        instructions
      end

      def transform_integer_node(node, used:)
        return [] unless used
        [PushIntInstruction.new(node.value)]
      end

      def transform_lambda_node(node, used:)
        instructions = transform_block_node(node, used: true, is_lambda: true)
        instructions << CreateLambdaInstruction.new
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_local_variable_and_write_node(node, used:)
        instructions = [
          VariableGetInstruction.new(node.name, default_to_nil: true),
          IfInstruction.new,
          transform_expression(node.value, used: true),
          VariableSetInstruction.new(node.name),
          ElseInstruction.new(:if),
          VariableGetInstruction.new(node.name),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_local_variable_operator_write_node(node, used:)
        instructions = [
          VariableGetInstruction.new(node.name, default_to_nil: true),
          transform_expression(node.value, used: true),
          PushArgcInstruction.new(1),
          SendInstruction.new(
            node.operator,
            receiver_is_self: false,
            with_block: false,
            file: node.location.path,
            line: node.location.start_line,
          ),
        ]
        instructions << DupInstruction.new if used
        instructions << VariableSetInstruction.new(node.name)
        instructions
      end

      def transform_local_variable_or_write_node(node, used:)
        instructions = [
          VariableGetInstruction.new(node.name, default_to_nil: true),
          IfInstruction.new,
          VariableGetInstruction.new(node.name),
          ElseInstruction.new(:if),
          transform_expression(node.value, used: true),
          VariableSetInstruction.new(node.name),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_local_variable_read_node(node, used:)
        return [] unless used
        VariableGetInstruction.new(node.name)
      end

      def transform_module_node(node, used:)
        instructions = []
        name, is_private, prep_instruction = constant_name(node.constant_path)
        instructions << prep_instruction
        instructions << DefineModuleInstruction.new(
          name: name,
          is_private: is_private,
          file: node.location.path,
          line: node.location.start_line,
        )
        instructions += transform_body(node.body, used: true)
        instructions << EndInstruction.new(:define_module)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_multi_write_node(node, used:)
        value = node.value

        instructions = [
          transform_expression(value, used: true),
          DupInstruction.new,
          ToArrayInstruction.new,
          MultipleAssignment.new(self, file: node.location.path, line: node.location.start_line).transform(node),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_next_node(node, used:)
        [
          transform_arguments_node_for_returnish(node.arguments, location: node.location),
          NextInstruction.new
        ]
      end

      def transform_nil_node(_, used:)
        return [] unless used
        [PushNilInstruction.new]
      end

      def transform_numbered_reference_read_node(node, used:)
        return [] unless used
        [
          PushLastMatchInstruction.new(to_s: false),
          DupInstruction.new,
          IfInstruction.new,
          PushIntInstruction.new(node.number),
          PushArgcInstruction.new(1),
          SendInstruction.new(
            :[],
            receiver_is_self: false,
            with_block: false,
            file: node.location.path,
            line: node.location.start_line,
          ),
          ElseInstruction.new(:if),
          PopInstruction.new,
          PushNilInstruction.new,
          EndInstruction.new(:if),
        ]
      end

      def transform_or_node(node, used:)
        instructions = [
          *transform_expression(node.left, used: true),
          DupInstruction.new,
          IfInstruction.new,
          ElseInstruction.new(:if),
          PopInstruction.new,
          *transform_expression(node.right, used: true),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_range_node(node, used:)
        instructions = [
          transform_expression(node.right || Prism.nil_node, used: true),
          transform_expression(node.left || Prism.nil_node, used: true),
          PushRangeInstruction.new(node.exclude_end?),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_rational_node(node, used:)
        return [] unless used

        value = node.value
        [
          PushIntInstruction.new(value.numerator),
          PushIntInstruction.new(value.denominator),
          PushRationalInstruction.new,
        ]
      end

      def transform_redo_node(*)
        [RedoInstruction.new]
      end

      def transform_regular_expression_node(node, used:)
        return [] unless used
        regexp = Regexp.new(node.content, node.options)
        PushRegexpInstruction.new(regexp)
      end

      def transform_retry_node(*)
        [RetryInstruction.new(id: @retry_context.last)]
      end

      def transform_return_node(node, used:) # rubocop:disable Lint/UnusedMethodArgument
        [
          transform_arguments_node_for_returnish(node.arguments, location: node.location),
          ReturnInstruction.new
        ]
      end

      def transform_self_node(_, used:)
        return [] unless used
        [PushSelfInstruction.new]
      end

      def transform_singleton_class_node(node, used:)
        instructions = [
          transform_expression(node.expression, used: true),
          WithSingletonInstruction.new,
          transform_body(node.body, used: true),
          EndInstruction.new(:with_singleton),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_source_file_node(node, used:)
        return [] unless used
        [PushStringInstruction.new(node.filepath)]
      end

      def transform_source_line_node(node, used:)
        return [] unless used
        [PushIntInstruction.new(node.location.start_line)]
      end

      def transform_splat_node(node, used:)
        transform_expression(node.expression, used: used)
      end

      def transform_statements_node(node, used:)
        transform_body(node.body, used: used)
      end

      def transform_string_node(node, used:)
        return [] unless used
        PushStringInstruction.new(node.unescaped)
      end

      def transform_super_node(node, used:)
        instructions = []

        # block handling
        if node.block.is_a?(Prism::BlockNode)
          with_block = true
          instructions << transform_block_node(
            node.block,
            used: true,
            is_lambda: is_lambda_call?(node)
          )
        elsif node.block.is_a?(Prism::BlockArgumentNode)
          with_block = true
          instructions << transform_expression(node.block, used: true)
        end

        instructions << PushSelfInstruction.new

        args = node.arguments&.arguments || []
        call_args = transform_call_args(args, with_block: with_block, instructions: instructions)
        instructions << SuperInstruction.new(
          args_array_on_stack: call_args.fetch(:args_array_on_stack),
          with_block: with_block,
          has_keyword_hash: call_args.fetch(:has_keyword_hash)
        )

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_symbol_node(node, used:)
        return [] unless used
        [PushSymbolInstruction.new(node.unescaped.to_sym)]
      end

      def transform_true_node(_, used:)
        return [] unless used
        [PushTrueInstruction.new]
      end

      def transform_yield_node(node, used:)
        arg_meta = transform_arguments_node_for_callish(node.arguments)
        instructions = arg_meta.fetch(:instructions)
        instructions << YieldInstruction.new(
          args_array_on_stack: arg_meta.fetch(:args_array_on_stack),
        )
        instructions << PopInstruction.new unless used
        instructions
      end

      # INDIVIDUAL EXPRESSIONS = = = = =
      # (in alphabetical order)

      def transform_alias(exp, used:)
        _, new_name, old_name = exp
        instructions = [transform_expression(new_name, used: true)]
        instructions << DupInstruction.new if used
        instructions << transform_expression(old_name, used: true)
        instructions << AliasInstruction.new
      end

      def transform_array_with_splat(elements)
        elements = elements.dup
        instructions = []

        # create array from items before the splat
        prior_to_splat_count = 0
        while elements.any? && elements.first.sexp_type != :splat_node
          instructions << transform_expression(elements.shift, used: true)
          prior_to_splat_count += 1
        end
        instructions << CreateArrayInstruction.new(count: prior_to_splat_count)

        # now add to the array the first splat item and everything after
        elements.each do |arg|
          if arg.sexp_type == :splat_node
            instructions << transform_expression(arg.expression, used: true)
            instructions << ArrayConcatInstruction.new
          else
            instructions << transform_expression(arg, used: true)
            instructions << ArrayPushInstruction.new
          end
        end

        instructions
      end

      def transform_autoload_const(exp, used:)
        _, name, path, *body = exp
        instructions = [
          AutoloadConstInstruction.new(name: name, path: path),
          transform_body(body, used: true),
          EndInstruction.new(:autoload_const),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_bare_hash(exp, used:)
        transform_hash(exp, bare: true, used: used)
      end

      def transform_block_argument_node(node, used:)
        return [] unless used
        [transform_expression(node.expression, used: true)]
      end

      def transform_block_node(node, used:, is_lambda:)
        arity = Arity.new(node.parameters, is_proc: !is_lambda).arity
        instructions = []
        instructions << DefineBlockInstruction.new(arity: arity)
        if is_lambda
          instructions << transform_block_args_for_lambda(node.parameters, used: true)
        else
          instructions << transform_block_args(node.parameters, used: true)
        end
        instructions << transform_expression(node.body || Prism.nil_node, used: true)
        instructions << EndInstruction.new(:define_block)
        instructions
      end

      def transform_block_args(exp, used:)
        transform_defn_args(exp, for_block: true, check_args: false, used: used)
      end

      def transform_block_args_for_lambda(exp, used:)
        transform_defn_args(exp, for_block: true, check_args: true, used: used)
      end

      def transform_block_args_for_for(exp, used:)
        transform_defn_args(exp, for_block: true, check_args: false, local_only: false, used: used)
      end

      def transform_call_args(args, with_block:, instructions: [])
        if args.size == 1 && args.first.type == :forwarding_arguments_node && !with_block
          instructions.unshift(PushBlockInstruction.new)
          with_block = true
        end

        if args.any? { |a| a.sexp_type == :splat_node }
          instructions << transform_array_with_splat(args)
          return {
            with_block_pass: !!with_block,
            args_array_on_stack: true,
            has_keyword_hash: args.last&.sexp_type == :bare_hash
          }
        end

        # special ... syntax
        if args.size == 1 && args.first.type == :forwarding_arguments_node
          instructions << PushArgsInstruction.new(
            for_block: false,
            min_count: nil,
            max_count: nil,
            spread: false,
            to_array: false,
          )
          return {
            instructions: instructions,
            with_block_pass: !!with_block,
            args_array_on_stack: true,
            has_keyword_hash: false,
            forward_args: true,
          }
        end

        args.each do |arg|
          instructions << transform_expression(arg, used: true)
        end

        has_keyword_hash = args.last&.sexp_type == :bare_hash

        instructions << PushArgcInstruction.new(args.size)

        {
          instructions: instructions,
          with_block_pass: !!with_block,
          args_array_on_stack: false,
          has_keyword_hash: has_keyword_hash,
        }
      end

      def transform_const(exp, used:)
        return [] unless used
        _, name = exp
        [
          PushSelfInstruction.new,
          ConstFindInstruction.new(name, strict: false),
        ]
      end

      def transform_defn_args(exp, used:, for_block: false, check_args: true, local_only: true)
        return [] unless used
        _, *args = exp

        instructions = []

        # special ... syntax
        if args.size == 1 && args.first.type == :forwarding_parameter_node
          # nothing to do
          return []
        end

        # &block pass
        if args.last&.type == :block_parameter_node
          block_arg = args.pop
          instructions << PushBlockInstruction.new
          instructions << VariableSetInstruction.new(block_arg.name, local_only: local_only)
        end

        has_complicated_args = args.any? do |arg|
          if arg.is_a?(::Prism::Node)
            arg.type != :required_parameter_node
          else
            arg.is_a?(Sexp) || arg.nil? || arg.start_with?('*')
          end
        end

        if args.count { |a| a.type == :required_parameter_node && a.name == :_ } > 1
          has_complicated_args = true
        end

        may_need_to_destructure_args_for_block = for_block && args.size > 1

        if has_complicated_args || may_need_to_destructure_args_for_block
          min_count = minimum_arg_count(args)
          max_count = maximum_arg_count(args)
          required_keywords = required_keywords(args)

          instructions << PopKeywordArgsInstruction.new if any_keyword_args?(args)

          if check_args
            argc = min_count == max_count ? min_count : min_count..max_count
            instructions << CheckArgsInstruction.new(positional: argc, keywords: required_keywords)
          end

          if required_keywords.any?
            instructions << CheckRequiredKeywordsInstruction.new(required_keywords)
          end

          instructions << PushArgsInstruction.new(
            for_block: for_block,
            min_count: min_count,
            max_count: max_count,
            spread: for_block && args.size > 1,
          )

          instructions << Args.new(
            self,
            local_only: local_only,
            file: exp.file,
            line: exp.line
          ).transform(exp.new(:args, *args))

          return instructions
        end

        if check_args
          instructions << CheckArgsInstruction.new(positional: args.size, keywords: [])
        end

        args.each_with_index do |arg, index|
          instructions << PushArgInstruction.new(index, nil_default: for_block)
          instructions << VariableSetInstruction.new(arg.name, local_only: local_only)
        end

        instructions
      end

      def transform_dregx(exp, used:)
        options = exp.pop if exp.last.is_a?(Integer)
        instructions = [
          transform_dstr(exp, used: true),
          StringToRegexpInstruction.new(options: options),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_dstr(exp, used:)
        _, start, *rest = exp
        instructions = [PushStringInstruction.new(start)]
        rest.each do |segment|
          case segment.sexp_type
          when :evstr
            _, value = segment
            instructions << transform_expression(value, used: true)
            instructions << StringAppendInstruction.new
          when :str
            instructions << transform_expression(segment, used: true)
            instructions << StringAppendInstruction.new
          else
            raise "unknown dstr segment: #{segment.inspect}"
          end
        end
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_dsym(exp, used:)
        instructions = [
          transform_dstr(exp, used: true),
          PushArgcInstruction.new(0),
          SendInstruction.new(
            :to_sym,
            receiver_is_self: false,
            with_block: false,
            file: exp.file,
            line: exp.line,
          ),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_dxstr(exp, used:)
        _, *parts = exp
        instructions = [
          transform_dstr(exp.new(:dstr, *parts), used: true),
          ShellInstruction.new,
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_ensure(exp, used:)
        _, body, ensure_body = exp
        raise_call = Prism.call_node(receiver: nil, name: :raise)
        instructions = [
          TryInstruction.new(discard_catch_result: true),
          transform_expression(body, used: true),
          CatchInstruction.new,
          transform_expression(ensure_body, used: true),
          transform_expression(raise_call, used: true),
          EndInstruction.new(:try),
          DupInstruction.new,
          PushRescuedInstruction.new,
          IfInstruction.new,
          ElseInstruction.new(:if),
          transform_expression(ensure_body, used: false),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_for_declare_args(args)
        instructions = []

        case args.sexp_type
        when :class_variable_target_node
          # Do nothing, no need to declare class variables.
        when :lasgn
          instructions << VariableDeclareInstruction.new(args[1])
        when :local_variable_target_node
          instructions << VariableDeclareInstruction.new(args.name)
        when :masgn
          args[1].elements.each do |arg|
            instructions += transform_for_declare_args(arg)
          end
        when :multi_target_node
          args.targets.each do |arg|
            instructions += transform_for_declare_args(arg)
          end
        else
          raise "I don't yet know how to declare this variable: #{args.inspect}"
        end

        instructions
      end

      def transform_gasgn(exp, used:)
        _, name, value = exp
        instructions = [transform_expression(value, used: true), GlobalVariableSetInstruction.new(name)]
        instructions << GlobalVariableGetInstruction.new(name) if used
        instructions
      end

      def transform_hash(exp, used:, bare: false)
        _, *items = exp
        instructions = []
        if items.any? { |a| a.sexp_type == :assoc_splat_node }
          instructions += transform_hash_with_assoc_splat(items, bare: bare)
        else
          items.each do |item|
            instructions << transform_expression(item, used: true)
          end
          instructions << CreateHashInstruction.new(count: items.size / 2, bare: bare)
        end
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_hash_with_assoc_splat(items, bare:)
        items = items.dup
        instructions = []

        # create hash from items before the splat
        prior_to_splat_count = 0
        while items.any? && items.first.sexp_type != :assoc_splat_node
          instructions << transform_expression(items.shift, used: true)
          prior_to_splat_count += 1
        end
        instructions << CreateHashInstruction.new(count: prior_to_splat_count / 2, bare: bare)

        # now add to the hash the first splat item and everything after
        while items.any?
          key = items.shift
          if key.sexp_type == :assoc_splat_node
            instructions << transform_expression(key.value, used: true)
            instructions << HashMergeInstruction.new
          else
            value = items.shift
            instructions << transform_expression(key, used: true)
            instructions << transform_expression(value, used: true)
            instructions << HashPutInstruction.new
          end
        end

        instructions
      end

      def transform_if(exp, used:)
        _, condition, true_expression, false_expression = exp
        true_instructions = transform_expression(true_expression || Prism.nil_node, used: true)
        false_instructions = transform_expression(false_expression || Prism.nil_node, used: true)
        instructions = [
          transform_expression(condition, used: true),
          IfInstruction.new,
          true_instructions,
          ElseInstruction.new(:if),
          false_instructions,
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_iter(exp, used:)
        _, call, args, body = exp
        is_lambda = is_lambda_call?(call)
        arity = Arity.new(args, is_proc: !is_lambda).arity
        instructions = []
        instructions << DefineBlockInstruction.new(arity: arity)
        if is_lambda_call?(call)
          instructions << transform_block_args_for_lambda(args, used: true)
        else
          instructions << transform_block_args(args, used: true)
        end
        instructions << transform_expression(body || Prism.nil_node, used: true)
        instructions << EndInstruction.new(:define_block)
        case call.sexp_type
        when :lambda
          instructions << transform_lambda(call, used: used)
        else
          raise "unexpected call: #{call.sexp_type.inspect}"
        end
        instructions
      end

      def transform_ivar(exp, used:)
        return [] unless used
        _, name = exp
        InstanceVariableGetInstruction.new(name)
      end

      def transform_lasgn(exp, used:)
        _, name, value = exp
        instructions = [
          VariableDeclareInstruction.new(name),
          transform_expression(value, used: true),
          VariableSetInstruction.new(name),
        ]
        instructions << VariableGetInstruction.new(name) if used
        instructions
      end

      def transform_lit(exp, used:)
        return [] unless used
        lit = if exp.is_a?(Sexp)
                exp[1]
              else
                exp
              end
        case lit
        when Integer
          PushIntInstruction.new(lit)
        when Float
          PushFloatInstruction.new(lit)
        when Rational
          [
            transform_lit(lit.numerator, used: true),
            transform_lit(lit.denominator, used: true),
            PushRationalInstruction.new,
          ]
        when Complex
          [
            transform_lit(lit.real, used: true),
            transform_lit(lit.imaginary, used: true),
            PushComplexInstruction.new,
          ]
        else
          raise "I don't yet know how to handle lit: \"#{lit.inspect}\" (#{exp.file}:#{exp.line}:#{exp.column})"
        end
      end

      def transform_lvar(exp, used:)
        return [] unless used
        _, name = exp
        VariableGetInstruction.new(name)
      end

      def transform_match_write(exp, used:)
        _, call, *locals = exp
        instructions = []
        instructions << transform_expression(call, used: used)
        instructions << PushLastMatchInstruction.new(to_s: false)
        instructions << IfInstruction.new

        # if match
        instructions << PushLastMatchInstruction.new(to_s: false)
        instructions << PushArgcInstruction.new(0)
        instructions << SendInstruction.new(
          :named_captures,
          receiver_is_self: false,
          with_block: false,
          file: exp.file,
          line: exp.line,
        )
        locals.each do |name|
          instructions << DupInstruction.new
          instructions << PushStringInstruction.new(name.to_s)
          instructions << PushArgcInstruction.new(1)
          instructions << SendInstruction.new(
            :[],
            receiver_is_self: false,
            with_block: false,
            file: exp.file,
            line: exp.line,
          )
          instructions << VariableSetInstruction.new(name)
        end
        instructions << PopInstruction.new # get rid of named captures

        # if no match
        instructions << ElseInstruction.new(:if)
        locals.each do |name|
          instructions << PushNilInstruction.new
          instructions << VariableSetInstruction.new(name)
        end
        instructions << EndInstruction.new(:if)

        instructions
      end

      def transform_not(exp, used:)
        _, value = exp
        instructions = [
          transform_expression(value, used: true),
          NotInstruction.new,
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_rescue(exp, used:)
        instructions = Rescue.new(self).transform(exp)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_str(exp, used:)
        return [] unless used
        _, str = exp
        PushStringInstruction.new(str)
      end

      def transform_svalue(exp, used:)
        _, svalue = exp
        instructions = []
        case svalue.sexp_type
        when :splat
          instructions << transform_expression(svalue, used: true)
          instructions << ArrayWrapInstruction.new
        when :array_node
          instructions << transform_array_node(svalue, used: true)
        else
          raise "unexpected svalue type: #{svalue.inspect}"
        end
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_undef(exp, used:)
        _, name = exp
        name = if name.type == :symbol_node
                 name.unescaped.to_sym
               else
                 # FIXME: doesn't work with real dynamic symbol :-(
                 # only works with: s(:dsym, "simple")
                 # We'll need UndefineMethodInstruction to pop its argument from the stack to fix this.
                 name.last
               end
        instructions = [
          UndefineMethodInstruction.new(name: name),
        ]
        instructions << PushNilInstruction.new if used
        instructions
      end

      def transform_until(exp, used:)
        _, condition, body, pre = exp
        transform_while(
          exp.new(
            :while,
            Prism.call_node(receiver: condition, name: :!),
            body,
            pre
          ),
          used: used
        )
      end

      def transform_while(exp, used:)
        _, condition, body, pre = exp
        body ||= Prism.nil_node

        instructions = [
          WhileInstruction.new(pre: pre),
          transform_expression(condition, used: true),
          WhileBodyInstruction.new,
          transform_expression(body, used: true),
          EndInstruction.new(:while),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_with_filename(exp, used:)
        depth_was = @depth
        @depth = 0
        _, filename, require_once, *body = exp
        instructions = [
          WithFilenameInstruction.new(filename, require_once: require_once),
          transform_body(body, used: true),
          EndInstruction.new(:with_filename),
        ]
        instructions << PopInstruction.new unless used
        @depth = depth_was
        instructions
      end

      def transform_xstr(exp, used:)
        _, command = exp
        instructions = [
          transform_str(exp.new(:str, command), used: true),
          ShellInstruction.new,
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      # HELPERS = = = = = = = = = = = = =

      # returns a set of [name, is_private, prep_instruction]
      # prep_instruction being the instruction(s) needed to get the owner of the constant
      def constant_name(name)
        prepper = ConstPrepper.new(name, pass: self)
        [prepper.name, prepper.private?, prepper.namespace]
      end

      def is_lambda_call?(exp)
        exp == s(:lambda) ||
          (exp.is_a?(::Prism::CallNode) && exp.receiver.nil? && exp.name == :lambda)
      end

      def is_inline_macro_call_node?(node)
        inline_enabled = @inline_cpp_enabled[node.location.path] ||
                         node.name == :__internal_inline_code__
        inline_enabled &&
          node.receiver.nil? &&
          INLINE_CPP_MACROS.include?(node.name)
      end

      def minimum_arg_count(args)
        args.count do |arg|
          arg.type == :required_parameter_node || arg.type == :multi_target_node
        end
      end

      def maximum_arg_count(args)
        # splat means no maximum
        return nil if args.any? { |arg| arg.type == :rest_parameter_node }

        args.count do |arg|
          %i[
            local_variable_target_node
            multi_target_node
            optional_parameter_node
            required_destructured_parameter_node
            required_parameter_node
          ].include?(arg.type)
        end
      end

      def any_keyword_args?(args)
        args.any? do |arg|
          arg.type == :keyword_parameter_node ||
            arg.type == :keyword_rest_parameter_node
        end
      end

      def required_keywords(args)
        args.filter_map do |arg|
          if arg.type == :keyword_parameter_node && !arg.value
            arg.name
          end
        end
      end

      def repl?
        @compiler_context[:repl]
      end

      # HACK: When using the REPL, the parser doesn't differentiate between method calls
      # and variable lookup. But we know which variables are defined in the REPL,
      # so we can convert the CallNode back to an LocalVariableReadNode as needed.
      def fix_repl_var_that_looks_like_call(node)
        return false unless node.type == :call_node && node.receiver.nil?
        return false unless @compiler_context[:vars].key?(node.name)

        return Sexp.new(:lvar, node.name)
      end

      def s(*items)
        sexp = Sexp.new
        items.each { |item| sexp << item }
        sexp
      end

      class << self
        def debug_instructions(instructions)
          instructions.each_with_index do |instruction, index|
            desc = "#{index} #{instruction}"
            puts desc
          end
        end
      end
    end
  end
end
