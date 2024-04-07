require_relative './args'
require_relative './arity'
require_relative './base_pass'
require_relative './const_prepper'
require_relative './multiple_assignment'

module Natalie
  class Compiler
    # This compiler pass transforms AST from the Parser into Intermediate
    # Representation, which we implement using Instructions.
    # You can debug this pass with the `-d p1` CLI flag.
    class Pass1 < BasePass
      def initialize(ast, compiler_context:, macro_expander:, loaded_file:)
        super()
        @ast = ast
        @compiler_context = compiler_context
        @required_ruby_files = @compiler_context[:required_ruby_files]
        @macro_expander = macro_expander

        # If any user code has required 'natalie/inline', then we enable
        # magical extra features. :-)
        @inline_cpp_enabled = @compiler_context[:inline_cpp_enabled]

        # We need a way to associate `retry` with the `rescue` block it
        # belongs to. Using a stack of object ids seems to work ok.
        @retry_context = []

        # We need to know if we're at the top level (left-most indent)
        # of a file, which enables certain macros.
        @depth = 0

        # Our MacroExpander and our REPL need to know local variables.
        @locals_stack = []

        # `next` and `break` need to know their enclosing scope type,
        # e.g. block vs while loop, so we'll use a stack to keep track.
        @next_or_break_context = []

        # This points to the current LoadedFile being compiled.
        @file = loaded_file
      end

      attr_reader :file

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
        case @ast.type
        when :program_node
          transform_program_node(@ast, used: used).flatten
        when :statements_node
          with_locals([]) do
            transform_statements_node(@ast, used: used).flatten
          end
        else
          raise 'unexpected AST input'
        end
      end

      def transform_program_node(node, used:)
        with_locals(node.locals) do
          transform_statements_node(node.statements, used: used).flatten
        end
      end

      def transform_expression(node, used:, **kwargs)
        case node
        when ::Prism::Node
          node = expand_macros(node) if node.type == :call_node
          return transform_expression(node, used: used) unless node.is_a?(::Prism::Node)
          @depth += 1 unless node.type == :statements_node
          method = "transform_#{node.type}"
          result = track_scope(node) do
            send(method, node, used: used, **kwargs)
          end
          @depth -= 1 unless node.type == :statements_node
          Array(result).flatten
        when Array
          if %i[load_file autoload_const].include?(node.first)
            # TODO: remove this kludge and change these fake node types to CallNode or something else
            @depth += 1
            method = "transform_#{node.first}_fake_node"
            result = send(method, node, used: used, **kwargs)
            @depth -= 1
            Array(result).flatten
          else
            raise "Unknown node type: #{node.inspect}"
          end
        else
          raise "Unknown node type: #{node.inspect}"
        end
      end

      def expand_macros(node)
        locals = @locals_stack.last
        @macro_expander.expand(node, locals: locals, depth: @depth, file: @file)
      end

      def transform_body(body, location:, used:)
        body = body.body if body.is_a?(Prism::StatementsNode)
        *body, last = body
        nil_node = Prism.nil_node(location: location)
        instructions = body.map { |exp| transform_expression(exp, used: false) }
        instructions << transform_expression(last || nil_node, used: used)
        instructions
      end

      private

      # INDIVIDUAL NODES = = = = =
      # (in alphabetical order)

      def transform_alias_global_variable_node(node, used:)
        instructions = [PushSymbolInstruction.new(node.new_name.name)]
        instructions << DupInstruction.new if used
        instructions << PushSymbolInstruction.new(node.old_name.name)
        instructions << AliasGlobalInstruction.new
      end

      def transform_alias_method_node(node, used:)
        instructions = [transform_expression(node.new_name, used: true)]
        instructions << DupInstruction.new if used
        instructions << transform_expression(node.old_name, used: true)
        instructions << AliasMethodInstruction.new
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

        if args.last&.type == :block_argument_node
          block_arg = args.pop
          instructions.unshift(transform_expression(block_arg.expression, used: true))
        end

        if args.size == 1 && args.first.type == :forwarding_arguments_node && !block
          instructions.unshift(PushBlockInstruction.new)
          block = true
        end

        if args.any? { |a| a.type == :splat_node }
          instructions << transform_array_elements_with_splat(args)
          return {
            instructions: instructions,
            with_block_pass: !!block,
            args_array_on_stack: true,
            has_keyword_hash: args.last&.type == :keyword_hash_node
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

        has_keyword_hash = args.last&.type == :keyword_hash_node

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

      def transform_array_elements_with_splat(elements)
        elements = elements.dup
        instructions = []

        # create array from items before the splat
        prior_to_splat_count = 0
        while elements.any? && elements.first.type != :splat_node
          instructions << transform_expression(elements.shift, used: true)
          prior_to_splat_count += 1
        end
        instructions << CreateArrayInstruction.new(count: prior_to_splat_count)

        # now add to the array the first splat item and everything after
        elements.each do |arg|
          if arg.type == :splat_node
            instructions << transform_expression(arg.expression, used: true)
            instructions << ArrayConcatInstruction.new
          else
            instructions << transform_expression(arg, used: true)
            instructions << ArrayPushInstruction.new
          end
        end

        instructions
      end

      def transform_array_node(node, used:)
        elements = node.elements
        instructions = []

        if node.elements.any? { |a| a.type == :splat_node }
          instructions += transform_array_elements_with_splat(elements)
        else
          elements.each do |element|
            instructions << transform_expression(element, used: true)
          end
          instructions << CreateArrayInstruction.new(count: elements.size)
        end

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_autoload_const_fake_node(exp, used:)
        _, name, path, *body = exp
        instructions = [
          AutoloadConstInstruction.new(name: name, path: path),
          transform_body(body, used: true, location: nil),
          EndInstruction.new(:autoload_const),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_back_reference_read_node(node, used:)
        return [] unless used
        case node.slice
        when '$`', "$'", '$+', '$&'
          [GlobalVariableGetInstruction.new(node.slice.to_sym)]
        else
          raise "unknown back ref read node: #{node.slice}"
        end
      end

      def transform_begin_node(node, used:)
        try_instruction = TryInstruction.new
        retry_id = try_instruction.object_id

        nil_node = Prism.nil_node(location: node.location)
        instructions = transform_expression(node.statements || nil_node, used: true)

        if node.rescue_clause
          instructions.unshift(try_instruction)
          instructions += [
            CatchInstruction.new,
            retry_context(retry_id) do
              transform_expression(node.rescue_clause, used: true)
            end,
            EndInstruction.new(:try)
          ]
        end

        if node.else_clause
          instructions += [
            PushRescuedInstruction.new,
            IfInstruction.new,
            # noop
            ElseInstruction.new(:if),
            PopInstruction.new, # we don't want the result of the try
            transform_expression(node.else_clause, used: true),
            EndInstruction.new(:if),
          ]
        end

        if node.ensure_clause
          raise_call = Prism.call_node(receiver: nil, name: :raise, location: node.ensure_clause.location)
          instructions.unshift(TryInstruction.new(discard_catch_result: true))
          instructions += [
            CatchInstruction.new,
            transform_expression(node.ensure_clause.statements, used: true),
            transform_expression(raise_call, used: true),
            EndInstruction.new(:try),
            transform_expression(node.ensure_clause.statements, used: false)
          ]
        end

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_block_args(exp, used:)
        transform_defn_args(exp, for_block: true, check_args: false, used: used)
      end

      def transform_block_args_for_for(exp, used:)
        transform_defn_args(exp, for_block: true, check_args: false, local_only: false, used: used)
      end

      def transform_block_args_for_lambda(exp, used:)
        transform_defn_args(exp, for_block: true, check_args: true, used: used)
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

        with_locals(node.locals) do
          body = node.body || Prism.nil_node(location: node.location)
          instructions << transform_expression(body, used: true)
        end

        instructions << EndInstruction.new(:define_block)
        instructions
      end

      def transform_break_node(node, used:)
        instructions = [
          transform_arguments_node_for_returnish(node.arguments, location: node.location)
        ]

        if %i[while_node until_node].include?(@next_or_break_context.last)
          instructions << BreakOutInstruction.new
        else
          instructions << BreakInstruction.new
        end

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_call_args(args, with_block:, instructions: [])
        if args.size == 1 && args.first.type == :forwarding_arguments_node && !with_block
          instructions.unshift(PushBlockInstruction.new)
          with_block = true
        end

        if args.any? { |a| a.type == :splat_node }
          instructions << transform_array_elements_with_splat(args)
          return {
            with_block_pass: !!with_block,
            args_array_on_stack: true,
            has_keyword_hash: args.last&.type == :keyword_hash_node
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

        has_keyword_hash = args.last&.type == :keyword_hash_node

        instructions << PushArgcInstruction.new(args.size)

        {
          instructions: instructions,
          with_block_pass: !!with_block,
          args_array_on_stack: false,
          has_keyword_hash: has_keyword_hash,
        }
      end

      def transform_call_node(node, used:, with_block: false)
        receiver = node.receiver
        message = node.name
        args = node.arguments&.arguments || []

        if is_inline_macro_call_node?(node)
          instructions = []
          if message == :__call__
            args[1..].reverse_each do |arg|
              instructions << transform_expression(arg, used: true)
            end
          end
          instructions << InlineCppInstruction.new(node)
          instructions << PopInstruction.new unless used
          return instructions
        end

        if (lambda_call_result = hack_lambda_call_node(node, used: used))
          return lambda_call_result
        end

        instructions = []

        # block handling
        if node.block.is_a?(Prism::BlockNode)
          with_block = true
          instructions << transform_expression(
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
          instructions << PopInstruction.new # pop receiver since it is unused
          instructions << PopInstruction.new if with_block # pop block since it is unused
          instructions << PushNilInstruction.new
          instructions << ElseInstruction.new(:if)
        end

        call_args = transform_call_args(args, with_block: with_block, instructions: instructions)
        with_block ||= call_args.fetch(:with_block_pass)

        instructions << SendInstruction.new(
          message,
          args_array_on_stack: call_args.fetch(:args_array_on_stack),
          receiver_is_self: receiver.nil? || receiver.is_a?(Prism::SelfNode),
          with_block: with_block,
          has_keyword_hash: call_args.fetch(:has_keyword_hash),
          forward_args: call_args[:forward_args],
          file: @file.path,
          line: node.location.start_line,
        )

        if node.safe_navigation?
          # close out our safe navigation `if` wrapper
          instructions << EndInstruction.new(:if)
        end

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_call_and_write_node(node, used:)
        obj = node.receiver

        # a.foo &&= 'bar'
        instructions = [

          # a.foo
          transform_expression(node.receiver, used: true),
          PushArgcInstruction.new(0),
          SendInstruction.new(
            node.read_name,
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          ),

          # duplicate for use by the if falsey case
          DupInstruction.new,

          # if a.foo
          IfInstruction.new,

          # remove duplicated value that was truthy
          PopInstruction.new,

          # a.foo=('bar')
          transform_expression(node.receiver, used: true),
          transform_expression(node.value, used: true),
          PushArgcInstruction.new(1),
          SendInstruction.new(
            node.write_name,
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          ),

          ElseInstruction.new(:if),
          # if !a.foo, return duplicated value

          EndInstruction.new(:if),
        ]

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_call_operator_write_node(node, used:)
        obj = node.receiver

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
            file: @file.path,
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
            file: @file.path,
            line: node.location.start_line,
          ),

          # obj.foo = new_value
          PushArgcInstruction.new(1),
          SendInstruction.new(
            writer,
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          ),
        ]

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_call_or_write_node(node, used:)
        obj = node.receiver

        # a.foo ||= 'bar'
        instructions = [

          # a.foo
          transform_expression(node.receiver, used: true),
          PushArgcInstruction.new(0),
          SendInstruction.new(
            node.read_name,
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
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
            file: @file.path,
            line: node.location.start_line,
          ),

          EndInstruction.new(:if),
        ]

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_case_node(node, used:)
        instructions = []

        if node.predicate
          instructions << transform_expression(node.predicate, used: true)
          node.conditions.each do |when_statement|
            # case a
            # when b, c, d
            # =>
            # if (b === a || c === a || d === a)
            options = when_statement.conditions
            body = when_statement.statements&.body

            options.each do |option|
              # Splats are handled in the backend.
              # For C++, it's done in the is_case_equal() function.
              if option.type == :splat_node
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
            instructions << transform_body(body, used: true, location: when_statement.location)
            instructions << ElseInstruction.new(:if)
          end
        else
          # glorified if-else
          node.conditions.each do |when_statement|
            # case
            # when a == b, b == c, c == d
            # =>
            # if (a == b || b == c || c == d)
            options = when_statement.conditions
            body = when_statement.statements&.body

            options = options[1..].reduce(options[0]) do |prev, option|
              Prism.or_node(left: prev, right: option, location: prev.location)
            end

            instructions << transform_expression(options, used: true)
            instructions << IfInstruction.new
            instructions << transform_body(body, used: true, location: when_statement.location)
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
          file: @file.path,
          line: node.location.start_line,
        )
        with_locals(node.locals) do
          instructions += transform_body(node.body, used: true, location: node.location)
        end
        instructions << EndInstruction.new(:define_class)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_class_variable_and_write_node(node, used:)
        instructions = [
          ClassVariableGetInstruction.new(node.name, default_to_nil: true),
          DupInstruction.new, # duplicate for falsey case
          IfInstruction.new,
          PopInstruction.new, # remove duplicated truthy value
          transform_expression(node.value, used: true),
          DupInstruction.new, # duplicate value for return
          ClassVariableSetInstruction.new(node.name),
          ElseInstruction.new(:if),
          EndInstruction.new(:if)
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_class_variable_operator_write_node(node, used:)
        instructions = [
          ClassVariableGetInstruction.new(node.name, default_to_nil: true),
          transform_expression(node.value, used: true),
          PushArgcInstruction.new(1),
          SendInstruction.new(
            node.operator,
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line
          ),
          ClassVariableSetInstruction.new(node.name)
        ]
        instructions << ClassVariableGetInstruction.new(node.name) if used
        instructions
      end

      def transform_class_variable_or_write_node(node, used:)
        instructions = [
          ClassVariableGetInstruction.new(node.name, default_to_nil: true),
          DupInstruction.new, # duplicate for truthy case
          IfInstruction.new,
          ElseInstruction.new(:if),
          PopInstruction.new, # remove duplicated falsey value
          transform_expression(node.value, used: true),
          DupInstruction.new, # duplicate value for return
          ClassVariableSetInstruction.new(node.name),
          EndInstruction.new(:if),
        ]
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

      def transform_constant_and_write_node(node, used:)
        instructions = [
          PushSelfInstruction.new,
          ConstFindInstruction.new(node.name, strict: false),
          DupInstruction.new,
          IfInstruction.new,
          PopInstruction.new,
          transform_expression(node.value, used: true),
          DupInstruction.new,
          PushSelfInstruction.new,
          ConstSetInstruction.new(node.name),
          ElseInstruction.new(:if),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_constant_or_write_node(node, used:)
        instructions = [
          IsDefinedInstruction.new(type: 'constant'),
          PushSelfInstruction.new,
          ConstFindInstruction.new(node.name, strict: false),
          EndInstruction.new(:is_defined),
          IfInstruction.new,
          PushSelfInstruction.new,
          ConstFindInstruction.new(node.name, strict: false),
          ElseInstruction.new(:if),
          transform_expression(node.value, used: true),
          DupInstruction.new,
          PushSelfInstruction.new,
          ConstSetInstruction.new(node.name),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
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
        instructions = [
          PushSelfInstruction.new,
          ConstFindInstruction.new(
            node.name,
            strict: false,
            file: @file.path,
            line: node.location.start_line
          ),
        ]
        instructions << PopInstruction.new unless used
        instructions
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
          DefineMethodInstruction.new(name: node.name, arity: arity, file: @file.path, line: node.location.start_line),
          transform_defn_args(node.parameters, used: true),
          with_locals(node.locals) do
            transform_body([node.body], used: true, location: node.location)
          end,
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

      def transform_defn_args(node, used:, for_block: false, check_args: true, local_only: true)
        return [] unless used

        node = node.parameters if node.is_a?(Prism::BlockParametersNode)

        args = case node
               when nil
                 []
               when Prism::ParametersNode
                 (
                   node.requireds +
                   [node.rest] +
                   node.optionals +
                   node.posts +
                   node.keywords +
                   [node.keyword_rest] +
                   [node.block]
                 ).compact
               else
                 [node]
               end

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
          arg.type != :required_parameter_node
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

          instructions << Args.new(self, local_only: local_only).transform(args)

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

      def transform_else_node(node, used:)
        raise 'unexpected ElseNode child count' if node.child_nodes.size != 1

        if (n = node.child_nodes.first)
          transform_expression(n, used: used)
        elsif used
          [PushNilInstruction.new]
        else
          []
        end
      end

      def transform_false_node(_, used:)
        return [] unless used
        [PushFalseInstruction.new]
      end

      def transform_float_node(node, used:)
        return [] unless used
        [PushFloatInstruction.new(node.value)]
      end

      def transform_for_declare_args(args)
        instructions = []

        case args.type
        when :class_variable_target_node
          # Do nothing, no need to declare class variables.
        when :local_variable_write_node
          instructions << VariableDeclareInstruction.new(args.name)
        when :local_variable_target_node
          instructions << VariableDeclareInstruction.new(args.name)
        when :multi_target_node
          targets = args.lefts + [args.rest].compact + args.rights
          targets.each do |arg|
            instructions += transform_for_declare_args(arg)
          end
        else
          raise "I don't yet know how to declare this variable: #{args.inspect}"
        end

        instructions
      end

      def transform_for_node(node, used:)
        instructions = transform_for_declare_args(node.index)
        instructions << DefineBlockInstruction.new(arity: 1)
        instructions += transform_block_args_for_for(node.index, used: true)
        instructions += transform_expression(node.statements, used: true)
        instructions << EndInstruction.new(:define_block)
        call = Prism.call_node(receiver: node.collection, name: :each, location: node.location)
        instructions << transform_call_node(call, used: used, with_block: true)
        instructions
      end

      def transform_forwarding_super_node(node, used:, with_block: false)
        instructions = []

        # block handling
        if node.block.is_a?(Prism::BlockNode)
          with_block = true
          instructions << transform_expression(
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
          DupInstruction.new, # duplicate for falsey case
          IfInstruction.new,
          PopInstruction.new, # remove duplicated truthy value
          transform_expression(node.value, used: true),
          DupInstruction.new, # duplicate value for return
          GlobalVariableSetInstruction.new(node.name),
          ElseInstruction.new(:if),
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
            file: @file.path,
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
          DupInstruction.new, # duplicate for truthy case
          IfInstruction.new,
          ElseInstruction.new(:if),
          PopInstruction.new, # remove duplicated falsey value
          transform_expression(node.value, used: true),
          DupInstruction.new, # duplicate value for return
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

      def transform_global_variable_write_node(node, used:)
        [
          transform_expression(node.value, used: true),
          used ? DupInstruction.new : nil,
          GlobalVariableSetInstruction.new(node.name)
        ].compact
      end

      def transform_hash_node(node, used:)
        instructions = []

        # create hash from elements before a splat
        prior_to_splat_count = 0
        node.elements.each do |element|
          break if element.type == :assoc_splat_node
          instructions << transform_expression(element.key, used: true)
          instructions << transform_expression(element.value, used: true)
          prior_to_splat_count += 1
        end
        instructions << CreateHashInstruction.new(count: prior_to_splat_count)

        # now, if applicable, add to the hash the splat element and everything after
        node.elements[prior_to_splat_count..].each do |element|
          if element.type == :assoc_splat_node
            instructions << transform_expression(element.value, used: true)
            instructions << HashMergeInstruction.new
          else
            instructions << transform_expression(element.key, used: true)
            instructions << transform_expression(element.value, used: true)
            instructions << HashPutInstruction.new
          end
        end

        instructions
      end

      def transform_if_node(node, used:)
        true_body = node.statements || Prism.nil_node(location: node.location)
        false_body = node.consequent || Prism.nil_node(location: node.location)
        true_instructions = transform_expression(true_body, used: true)
        false_instructions = transform_expression(false_body, used: true)
        instructions = [
          transform_expression(node.predicate, used: true),
          IfInstruction.new,
          true_instructions,
          ElseInstruction.new(:if),
          false_instructions,
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
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
          CreateComplexInstruction.new,
        ]
      end

      def transform_implicit_node(node, used:)
        transform_expression(node.value, used: used)
      end

      def transform_index_and_write_node(node, used:)
        obj = node.receiver
        key_args = node.arguments&.arguments || []

        instructions = [
          # stack before: []
          #  stack after: [obj]
          transform_expression(obj, used: true),

          # stack before: [obj]
          #  stack after: [obj, *keys]
          key_args.map { |arg| transform_expression(arg, used: true) },

          # stack before: [obj, *keys]
          #  stack after: [obj, *keys, obj]
          DupRelInstruction.new(key_args.size),

          # stack before: [obj, *keys, obj]
          #  stack after: [obj, *keys, obj, *keys]
          key_args.each_with_index.map { |_, index| DupRelInstruction.new(index + key_args.size) },

          # stack before: [obj, *keys, obj, *keys]
          #  stack after: [obj, *keys, old_value]
          PushArgcInstruction.new(key_args.size),
          SendInstruction.new(
            :[],
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          ),

          # stack before: [obj, *keys, old_value]
          #  stack after: [obj, *keys, old_value, old_value]
          DupInstruction.new,

          # if old_value
          # stack before: [obj, *keys, old_value, old_value]
          #  stack after: [obj, *keys, old_value]
          IfInstruction.new,

          # stack before: [obj, *keys, old_value]
          #  stack after: [obj, *keys]
          PopInstruction.new,

          # stack before: [obj, *keys]
          #  stack after: [obj, *keys, new_value]
          transform_expression(node.value, used: true),

          # obj[*keys] = new_value
          # stack before: [obj, *keys, new_value]
          #  stack after: [result_of_send]
          PushArgcInstruction.new(key_args.size + 1),
          SendInstruction.new(
            :[]=,
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          ),

          # old_value is falsey...
          ElseInstruction.new(:if),

          # don't need the extra key(s) after all :-)
          # stack before: [obj, *keys, old_value]
          #  stack after: [obj, old_value]
          key_args.map do
            [
              SwapInstruction.new, # move value above duplicated key
              PopInstruction.new, # get rid of duplicated key
            ]
          end,

          # don't need the obj now
          # stack before: [obj, old_value]
          #  stack after: [old_value]
          SwapInstruction.new,
          PopInstruction.new,

          EndInstruction.new(:if),
        ]

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_index_operator_write_node(node, used:)
        obj = node.receiver
        key_args = node.arguments&.arguments || []

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
            file: @file.path,
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
            file: @file.path,
            line: node.location.start_line,
          ),

          # obj[*keys] = new_value
          PushArgcInstruction.new(key_args.size + 1),
          SendInstruction.new(
            :[]=,
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          ),
        ]

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_index_or_write_node(node, used:)
        obj = node.receiver
        key_args = node.arguments&.arguments || []

        instructions = [
          # stack before: []
          #  stack after: [obj]
          transform_expression(obj, used: true),

          # stack before: [obj]
          #  stack after: [obj, *keys]
          key_args.map { |arg| transform_expression(arg, used: true) },

          # stack before: [obj, *keys]
          #  stack after: [obj, *keys, obj]
          DupRelInstruction.new(key_args.size),

          # stack before: [obj, *keys, obj]
          #  stack after: [obj, *keys, obj, *keys]
          key_args.each_with_index.map { |_, index| DupRelInstruction.new(index + key_args.size) },

          # stack before: [obj, *keys, obj, *keys]
          #  stack after: [obj, *keys, old_value]
          PushArgcInstruction.new(key_args.size),
          SendInstruction.new(
            :[],
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          ),

          # stack before: [obj, *keys, old_value]
          #  stack after: [obj, *keys, old_value, old_value]
          DupInstruction.new,

          # if old_value
          # stack before: [obj, *keys, old_value, old_value]
          #  stack after: [obj, *keys, old_value]
          IfInstruction.new,

          # don't need the extra key(s) after all :-)
          # stack before: [obj, *keys, old_value]
          #  stack after: [obj, old_value]
          key_args.map do
            [
              SwapInstruction.new, # move value above duplicated key
              PopInstruction.new, # get rid of duplicated key
            ]
          end,

          # don't need the obj now
          # stack before: [obj, old_value]
          #  stack after: [old_value]
          SwapInstruction.new,
          PopInstruction.new,

          # old_value is falsey...
          ElseInstruction.new(:if),

          # stack before: [obj, *keys, old_value]
          #  stack after: [obj, *keys]
          PopInstruction.new,

          # stack before: [obj, *keys]
          #  stack after: [obj, *keys, new_value]
          transform_expression(node.value, used: true),

          # obj[*keys] = new_value
          # stack before: [obj, *keys, new_value]
          #  stack after: [result_of_send]
          PushArgcInstruction.new(key_args.size + 1),
          SendInstruction.new(
            :[]=,
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          ),

          EndInstruction.new(:if),
        ]

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_instance_variable_and_write_node(node, used:)
        instructions = [
          InstanceVariableGetInstruction.new(node.name),
          DupInstruction.new, # duplicate for falsey case
          IfInstruction.new,
          PopInstruction.new, # remove duplicated truthy value
          transform_expression(node.value, used: true),
          DupInstruction.new, # duplicate value for return
          InstanceVariableSetInstruction.new(node.name),
          ElseInstruction.new(:if),
          EndInstruction.new(:if)
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
            file: @file.path,
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
          DupInstruction.new, # duplicate for truthy case
          IfInstruction.new,
          ElseInstruction.new(:if),
          PopInstruction.new, # remove duplicated falsey value
          transform_expression(node.value, used: true),
          DupInstruction.new, # duplicate value for return
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

      def transform_interpolated_regular_expression_node(node, used:)
        instructions = transform_interpolated_stringish_node(node, used: true, unescaped: false)
        instructions << StringToRegexpInstruction.new(options: node.options)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_interpolated_string_node(node, used:)
        transform_interpolated_stringish_node(node, used: true, unescaped: true)
      end

      def transform_interpolated_stringish_node(node, used:, unescaped:)
        parts = node.parts.dup

        encoding = @file.encoding
        parts.each do |part|
          if part.respond_to?(:forced_utf8_encoding?) && part.forced_utf8_encoding?
            encoding = Encoding::UTF_8
            break
          elsif part.respond_to?(:forced_binary_encoding?) && part.forced_binary_encoding?
            encoding = Encoding::ASCII_8BIT
            break
          end
        end

        starter = if parts.first.type == :string_node
                    part = parts.shift
                    PushStringInstruction.new(unescaped ? part.unescaped : part.content, encoding: encoding)
                  else
                    PushStringInstruction.new('', encoding: encoding)
                  end

        instructions = [starter]

        parts.each do |part|
          case part
          when Prism::StringNode
            instructions << PushStringInstruction.new(unescaped ? part.unescaped : part.content, encoding: encoding)
          when Prism::EmbeddedStatementsNode
            if part.statements.nil?
              instructions << PushStringInstruction.new('', encoding: encoding)
            else
              instructions << transform_expression(part.statements, used: true)
            end
          when Prism::EmbeddedVariableNode
            instructions << transform_expression(part.variable, used: true)
          when Prism::InterpolatedStringNode
            instructions << transform_expression(part, used: true)
          else
            raise "unknown interpolated string segment: #{part.inspect}"
          end
          instructions << StringAppendInstruction.new
        end

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_interpolated_symbol_node(node, used:)
        instructions = [
          transform_interpolated_stringish_node(node, used: true, unescaped: false),
          PushArgcInstruction.new(0),
          SendInstruction.new(
            :to_sym,
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          )
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_interpolated_x_string_node(node, used:)
        instructions = [
          transform_interpolated_stringish_node(node, used: true, unescaped: false),
          ShellInstruction.new
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      alias transform_keyword_hash_node transform_hash_node

      def transform_lambda_node(node, used:)
        instructions = track_scope(node) do
          transform_block_node(node, used: true, is_lambda: true)
        end
        instructions << CreateLambdaInstruction.new(file: @file.path, line: node.location.start_line)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_load_file_fake_node(exp, used:)
        depth_was = @depth
        _, filename, require_once = exp
        loaded_file = @required_ruby_files.fetch(filename)

        unless loaded_file.instructions
          file_was = @file
          @file = loaded_file
          loaded_file.instructions = :generating # set this to avoid endless loop
          @depth = 0
          loaded_file.instructions = transform_expression(loaded_file.ast, used: true)
          @depth = depth_was
          @file = file_was
        end

        instructions = [
          LoadFileInstruction.new(filename, require_once: require_once),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_local_variable_and_write_node(node, used:)
        instructions = [
          VariableGetInstruction.new(node.name, default_to_nil: true),
          DupInstruction.new, # duplicate for falsey case
          IfInstruction.new,
          PopInstruction.new, # remove duplicated truthy value
          transform_expression(node.value, used: true),
          DupInstruction.new, # duplicate value for return
          VariableSetInstruction.new(node.name),
          ElseInstruction.new(:if),
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
            file: @current_path,
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
          DupInstruction.new, # duplicate for truthy case
          IfInstruction.new,
          ElseInstruction.new(:if),
          PopInstruction.new, # remove duplicated falsey value
          transform_expression(node.value, used: true),
          DupInstruction.new, # duplicate value for return
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

      def transform_local_variable_write_node(node, used:)
        instructions = [
          VariableDeclareInstruction.new(node.name),
          transform_expression(node.value, used: true),
        ]
        instructions << DupInstruction.new if used
        instructions << VariableSetInstruction.new(node.name)
        instructions
      end

      def transform_match_last_line_node(node, used:)
        regexp = Regexp.new(node.unescaped, node.options)
        instructions = [
          PushRegexpInstruction.new(regexp),
          GlobalVariableGetInstruction.new(:$_),
          PushArgcInstruction.new(1),
          SendInstruction.new(
            :=~,
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          ),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_match_write_node(node, used:)
        instructions = []
        instructions << transform_expression(node.call, used: used)
        instructions << PushLastMatchInstruction.new
        instructions << IfInstruction.new

        # if match
        instructions << PushLastMatchInstruction.new
        instructions << PushArgcInstruction.new(0)
        instructions << SendInstruction.new(
          :named_captures,
          receiver_is_self: false,
          with_block: false,
          file: @file.path,
          line: node.location.start_line,
        )
        node.targets.each do |target|
          raise "I cannot yet handle target: #{target.inspect}" unless target.is_a?(::Prism::LocalVariableTargetNode)
          name = target.name
          instructions << DupInstruction.new
          instructions << PushStringInstruction.new(name.to_s)
          instructions << PushArgcInstruction.new(1)
          instructions << SendInstruction.new(
            :[],
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          )
          instructions << VariableSetInstruction.new(name)
        end
        instructions << PopInstruction.new # get rid of named captures

        # if no match
        instructions << ElseInstruction.new(:if)
        node.targets.each do |target|
          raise "I cannot yet handle target: #{target.inspect}" unless target.is_a?(::Prism::LocalVariableTargetNode)
          instructions << PushNilInstruction.new
          instructions << VariableSetInstruction.new(target.name)
        end
        instructions << EndInstruction.new(:if)

        instructions
      end

      def transform_module_node(node, used:)
        instructions = []
        name, is_private, prep_instruction = constant_name(node.constant_path)
        instructions << prep_instruction
        instructions << DefineModuleInstruction.new(
          name: name,
          is_private: is_private,
          file: @file.path,
          line: node.location.start_line,
        )
        with_locals(node.locals) do
          instructions += transform_body(node.body, used: true, location: node.location)
        end
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
          MultipleAssignment.new(self, file: @file.path, line: node.location.start_line).transform(node),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_next_node(node, used:)
        if %i[while_node until_node].include?(@next_or_break_context.last)
          return [ContinueInstruction.new]
        end

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
          PushLastMatchInstruction.new,
          DupInstruction.new,
          IfInstruction.new,
          PushIntInstruction.new(node.number),
          PushArgcInstruction.new(1),
          SendInstruction.new(
            :[],
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
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

      def transform_parentheses_node(node, used:)
        if node.body
          transform_expression(node.body, used: used)
        elsif used
          [PushNilInstruction.new]
        else
          []
        end
      end

      def transform_range_node(node, used:)
        instructions = [
          transform_expression(node.right || Prism.nil_node(location: node.location), used: true),
          transform_expression(node.left || Prism.nil_node(location: node.location), used: true),
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
        regexp = Regexp.new(node.unescaped, node.options)
        PushRegexpInstruction.new(regexp)
      end

      def transform_rescue_modifier_node(node, used:)
        instructions = [
          TryInstruction.new,
          transform_expression(node.expression, used: true),
          CatchInstruction.new,
          PushSelfInstruction.new,
          ConstFindInstruction.new(:StandardError, strict: false),
          CreateArrayInstruction.new(count: 1),
          MatchExceptionInstruction.new,
          IfInstruction.new,
          transform_expression(node.rescue_expression, used: true),
          ElseInstruction.new(:if),
          PushSelfInstruction.new,
          PushArgcInstruction.new(0),
          SendInstruction.new(
            :raise,
            receiver_is_self: false,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          ),
          EndInstruction.new(:if),
          EndInstruction.new(:try),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_rescue_node(node, used:)
        exceptions = node.exceptions.dup
        unless exceptions.any?
          exceptions << Prism.constant_read_node(name: :StandardError, location: node.location)
        end

        instructions = [
          exceptions.map { |n| transform_expression(n, used: true) },
          CreateArrayInstruction.new(count: exceptions.size),
          MatchExceptionInstruction.new,
          IfInstruction.new,
          transform_rescue_reference_node(node.reference),
          transform_body(node.statements, used: true, location: node.location),
        ]

        instructions << ElseInstruction.new(:if)
        if node.consequent
          instructions += transform_expression(node.consequent, used: true)
        else
          instructions += [
            PushSelfInstruction.new,
            PushArgcInstruction.new(0),
            SendInstruction.new(
              :raise,
              receiver_is_self: true,
              with_block: false,
              file: @file.path,
              line: node.location.start_line,
            )
          ]
        end
        instructions << EndInstruction.new(:if)

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_rescue_reference_node(node)
        return [] if node.nil?

        instructions = [GlobalVariableGetInstruction.new(:$!)]

        case node
        when ::Prism::ClassVariableTargetNode
          instructions << ClassVariableSetInstruction.new(node.name)
        when ::Prism::ConstantTargetNode, ::Prism::ConstantPathTargetNode
          prepper = ConstPrepper.new(node, pass: self)
          instructions << [
            prepper.namespace,
            ConstSetInstruction.new(prepper.name)
          ]
        when ::Prism::GlobalVariableTargetNode
          instructions << GlobalVariableSetInstruction.new(node.name)
        when ::Prism::InstanceVariableTargetNode
          instructions << InstanceVariableSetInstruction.new(node.name)
        when ::Prism::LocalVariableTargetNode
          instructions << VariableSetInstruction.new(node.name)
        else
          raise "unhandled reference node for rescue: #{node.inspect}"
        end

        instructions
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
          transform_body(node.body, used: true, location: node.location),
          EndInstruction.new(:with_singleton),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_source_encoding_node(node, used:)
        instructions = [
          PushObjectClassInstruction.new,
          ConstFindInstruction.new(:Encoding, strict: true),
          PushStringInstruction.new(@file.encoding.name),
          PushArgcInstruction.new(1),
          SendInstruction.new(
            :find,
            receiver_is_self: true,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          ),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_source_file_node(node, used:)
        return [] unless used
        [PushStringInstruction.new(node.filepath, encoding: @file.encoding)]
      end

      def transform_source_line_node(node, used:)
        return [] unless used
        [PushIntInstruction.new(node.location.start_line)]
      end

      def transform_splat_node(node, used:)
        transform_expression(node.expression, used: used)
      end

      def transform_statements_node(node, used:)
        transform_body(node.body, used: used, location: node.location)
      end

      def transform_string_concat_node(node, used:)
        instructions = [
          transform_expression(node.left, used: true),
          transform_expression(node.right, used: true),
          StringAppendInstruction.new
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_string_node(node, used:)
        return [] unless used

        encoding = encoding_for_string_node(node)
        PushStringInstruction.new(node.unescaped, encoding: encoding)
      end

      def transform_super_node(node, used:)
        instructions = []

        # block handling
        if node.block.is_a?(Prism::BlockNode)
          with_block = true
          instructions << transform_expression(
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

      def transform_undef_node(node, used:)
        instructions = node.names.flat_map do |name|
          [
            transform_expression(name, used: true),
            UndefineMethodInstruction.new
          ]
        end

        instructions << PushNilInstruction.new if used
        instructions
      end

      def transform_unless_node(node, used:)
        true_body = node.statements || Prism.nil_node(location: node.location)
        false_body = node.consequent || Prism.nil_node(location: node.location)
        true_instructions = transform_expression(true_body, used: true)
        false_instructions = transform_expression(false_body, used: true)
        instructions = [
          transform_expression(node.predicate, used: true),
          IfInstruction.new,
          false_instructions,
          ElseInstruction.new(:if),
          true_instructions,
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_until_node(node, used:)
        pre = !node.begin_modifier?

        instructions = [
          WhileInstruction.new(pre: pre),
          transform_expression(node.predicate, used: true),
          PushArgcInstruction.new(0),
          SendInstruction.new(
            :!,
            receiver_is_self: true,
            with_block: false,
            file: @file.path,
            line: node.location.start_line,
          ),
          WhileBodyInstruction.new,
          transform_expression(node.statements || Prism.nil_node(location: node.location), used: true),
          EndInstruction.new(:while),
        ]

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_while_node(node, used:)
        pre = !node.begin_modifier?

        instructions = [
          WhileInstruction.new(pre: pre),
          transform_expression(node.predicate, used: true),
          WhileBodyInstruction.new,
          transform_expression(node.statements || Prism.nil_node(location: node.location), used: true),
          EndInstruction.new(:while),
        ]

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_x_string_node(node, used:)
        instructions = [
          PushStringInstruction.new(node.unescaped),
          ShellInstruction.new,
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_yield_node(node, used:)
        arg_meta = transform_arguments_node_for_callish(node.arguments)
        instructions = arg_meta.fetch(:instructions)
        instructions << YieldInstruction.new(
          args_array_on_stack: arg_meta.fetch(:args_array_on_stack),
          has_keyword_hash: arg_meta.fetch(:has_keyword_hash),
        )
        instructions << PopInstruction.new unless used
        instructions
      end

      # HELPERS = = = = = = = = = = = = =

      def hack_lambda_call_node(node, used:)
        return nil unless is_lambda_call?(node)

        if node.block.is_a?(Prism::BlockNode)
          # NOTE: We need Kernel#lambda to behave just like the stabby
          # lambda (->) operator so we can attach the "break point" to
          # it. I realize this is a bit of a hack and if someone wants
          # to alias Kernel#lambda, then their method will create a
          # broken lambda, i.e. calling `break` won't work correctly.
          # Maybe someday we can think of a better way to handle this...
          return transform_lambda_node(node.block, used: used)
        end

        # As of Ruby 3.3, passing a block argument to Kernel#lambda
        # is no longer supported.
        if node.block.is_a?(Prism::BlockArgumentNode)
          return [
            PushSelfInstruction.new,
            PushSelfInstruction.new,
            ConstFindInstruction.new(:ArgumentError, strict: false),
            PushStringInstruction.new('the lambda method requires a literal block'),
            PushArgcInstruction.new(2),
            SendInstruction.new(
              :raise,
              receiver_is_self: true,
              with_block:       false,
              file:             @file.path,
              line:             node.location.start_line,
            ),
          ]
        end

        nil
      end

      def encoding_for_string_node(node)
        if node.forced_utf8_encoding?
          Encoding::UTF_8
        elsif node.forced_binary_encoding?
          Encoding::ASCII_8BIT
        else
          @file.encoding
        end
      end

      def retry_context(id)
        @retry_context << id
        yield
      ensure
        @retry_context.pop
      end

      def next_or_break_context(node)
        case node
        when ::Prism::WhileNode, ::Prism::UntilNode, ::Prism::BlockNode, ::Prism::DefNode
          @next_or_break_context << node.type
          result = yield
          @next_or_break_context.pop
          result
        else
          yield
        end
      end

      def track_scope(...)
        # NOTE: we may have other contexts to track here later
        next_or_break_context(...)
      end

      # returns a set of [name, is_private, prep_instruction]
      # prep_instruction being the instruction(s) needed to get the owner of the constant
      def constant_name(name)
        prepper = ConstPrepper.new(name, pass: self)
        [prepper.name, prepper.private?, prepper.namespace]
      end

      def is_lambda_call?(node)
        node.is_a?(::Prism::CallNode) &&
          node.receiver.nil? &&
          node.name == :lambda &&
          (node.arguments.nil? || node.arguments.arguments.empty?)
      end

      def is_inline_macro_call_node?(node)
        inline_enabled = @inline_cpp_enabled[@file.path] ||
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
            required_parameter_node
          ].include?(arg.type)
        end
      end

      def any_keyword_args?(args)
        args.any? do |arg|
          arg.is_a?(::Prism::RequiredKeywordParameterNode) ||
            arg.is_a?(::Prism::OptionalKeywordParameterNode) ||
            arg.is_a?(::Prism::KeywordRestParameterNode)
        end
      end

      def required_keywords(args)
        args.filter_map do |arg|
          if arg.is_a?(::Prism::RequiredKeywordParameterNode)
            arg.name
          end
        end
      end

      def with_locals(locals)
        @locals_stack << locals
        yield
      ensure
        @locals_stack.pop
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
