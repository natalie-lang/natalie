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
      def initialize(ast, inline_cpp_enabled:, compiler_context:)
        super()
        @ast = ast
        @compiler_context = compiler_context

        # If any user code has required 'natalie/inline', then we enable
        # magical extra features. :-)
        @inline_cpp_enabled = inline_cpp_enabled

        # We need a way to associate `retry` with the `rescue` block it
        # belongs to. Using a stack of object ids seems to work ok.
        # See the Rescue class for how it's used.
        @retry_context = []
      end

      INLINE_CPP_MACROS = %i[
        __call__
        __constant__
        __cxx_flags__
        __define_method__
        __function__
        __inline__
        __ld_flags__
      ].freeze

      # pass used: true to leave the final result on the stack
      def transform(used: true)
        raise 'unexpected AST input' unless @ast.sexp_type == :block
        transform_block(@ast, used: used).flatten
      end

      def transform_expression(exp, used:)
        case exp
        when Sexp
          method = "transform_#{exp.sexp_type}"
          Array(send(method, exp, used: used)).flatten
        else
          raise "Unknown expression type: #{exp.inspect}"
        end
      end

      def transform_body(body, used:)
        *body, last = body
        instructions = body.map { |exp| transform_expression(exp, used: false) }
        instructions << transform_expression(last || s(:nil), used: used)
        instructions
      end

      def retry_context(id)
        @retry_context << id
        yield
      ensure
        @retry_context.pop
      end

      private

      # INDIVIDUAL EXPRESSIONS = = = = =
      # (in alphabetical order)

      def transform_alias(exp, used:)
        _, new_name, old_name = exp
        instructions = [transform_expression(new_name, used: true)]
        instructions << DupInstruction.new if used
        instructions << transform_expression(old_name, used: true)
        instructions << AliasInstruction.new
      end

      def transform_and(exp, used:)
        _, lhs, rhs = exp
        lhs_instructions = transform_expression(lhs, used: true)
        rhs_instructions = transform_expression(rhs, used: true)
        instructions = [
          lhs_instructions,
          DupInstruction.new,
          IfInstruction.new,
          PopInstruction.new,
          rhs_instructions,
          ElseInstruction.new(:if),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_array(exp, used:)
        _, *items = exp
        instructions = []
        if items.any? { |a| a.sexp_type == :splat }
          instructions += transform_array_with_splat(items)
        else
          items.each do |item|
            instructions << transform_expression(item, used: true)
          end
          instructions << CreateArrayInstruction.new(count: items.size)
        end
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_array_with_splat(items)
        items = items.dup
        instructions = []

        # create array from items before the splat
        prior_to_splat_count = 0
        while items.any? && items.first.sexp_type != :splat
          instructions << transform_expression(items.shift, used: true)
          prior_to_splat_count += 1
        end
        instructions << CreateArrayInstruction.new(count: prior_to_splat_count)

        # now add to the array the first splat item and everything after
        items.each do |arg|
          if arg.sexp_type == :splat
            _, value = arg
            instructions << transform_expression(value, used: true)
            instructions << ArrayConcatInstruction.new
          else
            instructions << transform_expression(arg, used: true)
            instructions << ArrayPushInstruction.new
          end
        end

        instructions
      end

      def transform_attrasgn(exp, used:)
        _, receiver, message, *args = exp
        transform_call(exp.new(:call, receiver, message, *args), used: used)
      end

      def transform_back_ref(exp, used:)
        return [] unless used
        _, name = exp
        raise "Unknown back ref: #{name.inspect}" unless name == :&
        [PushLastMatchInstruction.new(to_s: true)]
      end

      def transform_nth_ref(exp, used:)
        return [] unless used
        _, num = exp
        [
          PushLastMatchInstruction.new(to_s: false),
          DupInstruction.new,
          IfInstruction.new,
          PushIntInstruction.new(num),
          PushArgcInstruction.new(1),
          DupRelInstruction.new(2),
          SendInstruction.new(:[], receiver_is_self: false, with_block: false, file: exp.file, line: exp.line),
          ElseInstruction.new(:if),
          PopInstruction.new,
          PushNilInstruction.new,
          EndInstruction.new(:if),
        ]
      end

      def transform_bare_hash(exp, used:)
        transform_hash(exp, bare: true, used: used)
      end

      def transform_block(exp, used:)
        _, *body = exp
        transform_body(body, used: used)
      end

      def transform_block_args(exp, used:)
        transform_defn_args(exp, for_block: true, check_args: false, used: used)
      end

      def transform_block_args_for_lambda(exp, used:)
        transform_defn_args(exp, for_block: true, check_args: true, used: used)
      end

      def transform_break(exp, used:) # rubocop:disable Lint/UnusedMethodArgument
        _, value = exp
        value ||= s(:nil)
        [
          transform_expression(value, used: true),
          BreakInstruction.new,
        ]
      end

      def transform_call(exp, used:, with_block: false)
        _, receiver, message, *args = exp

        if repl? && (new_exp = fix_repl_var_that_looks_like_call(exp))
          return transform_expression(new_exp, used: used)
        end

        if @inline_cpp_enabled && receiver.nil? && INLINE_CPP_MACROS.include?(message)
          instructions = []
          if message == :__call__
            args[1..].reverse_each do |arg|
              instructions << transform_expression(arg, used: true)
            end
          end
          instructions << InlineCppInstruction.new(exp)
          return instructions
        end

        if receiver.nil? && message == :lambda && args.empty?
          # NOTE: We need Kernel#lambda to behave just like the stabby
          # lambda (->) operator so we can attach the "break point" to
          # it. I realize this is a bit of a hack and if someone wants
          # to alias Kernel#lambda, then their method will create a
          # broken lambda, i.e. calling `break` won't work correctly.
          # Maybe someday we can think of a better way to handle this...
          return transform_lambda(exp.new(:lambda), used: used)
        end

        if receiver.nil? && message == :block_given? && !with_block
          return [
            PushBlockInstruction.new,
            transform_call(exp, used: used, with_block: true),
          ]
        end

        call_args = transform_call_args(args)
        instructions = call_args.fetch(:instructions)
        with_block ||= call_args.fetch(:with_block_pass)

        if receiver.nil?
          instructions << PushSelfInstruction.new
        else
          instructions << transform_expression(receiver, used: true)
        end

        instructions << SendInstruction.new(
          message,
          args_array_on_stack: call_args.fetch(:args_array_on_stack),
          receiver_is_self: receiver.nil?,
          with_block: with_block,
          has_keyword_hash: call_args.fetch(:has_keyword_hash),
          file: exp.file,
          line: exp.line,
        )
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_call_args(args)
        instructions = []

        if args.last&.sexp_type == :block_pass
          _, block = args.pop
          instructions << transform_expression(block, used: true)
        end

        if args.any? { |a| a.sexp_type == :splat }
          instructions << transform_array_with_splat(args)
          return {
            instructions: instructions,
            with_block_pass: !!block,
            args_array_on_stack: true,
            has_keyword_hash: false, # TODO
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

      def transform_case(exp, used:)
        _, var, *whens, else_block = exp
        return transform_expression(else_block, used: used) if whens.empty?
        instructions = []
        if var
          instructions << transform_expression(var, used: true)
          whens.each do |when_statement|
            # case a
            # when b, c, d
            # =>
            # if (b === a || c === a || d === a)
            _, options_array, *body = when_statement
            _, *options = options_array

            options.each do |option|
              # Splats are handled in the backend.
              # For C++, it's done in the is_case_equal() function.
              if option.sexp_type == :splat
                _, option = option
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
          whens.each do |when_statement|
            # case
            # when a == b, b == c, c == d
            # =>
            # if (a == b || b == c || c == d)
            _, options, *body = when_statement

            # s(:array, option1, option2, ...)
            # =>
            # s(:or, option1, s(:or, option2, ...))
            options = options[2..].reduce(options[1]) { |prev, option| s(:or, prev, option) }
            instructions << transform_expression(options, used: true)
            instructions << IfInstruction.new
            instructions << transform_body(body, used: true)
            instructions << ElseInstruction.new(:if)
          end
        end

        instructions << (else_block.nil? ? PushNilInstruction.new : transform_expression(else_block, used: true))

        instructions << [EndInstruction.new(:if)] * whens.length

        # The case value is never popped during comparison, so we have to pop it here.
        instructions << [SwapInstruction.new, PopInstruction.new] if var

        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_cdecl(exp, used:)
        _, name, value = exp
        instructions = [transform_expression(value, used: true)]
        name, prep_instruction = constant_name(name)
        instructions << prep_instruction
        instructions << ConstSetInstruction.new(name)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_class(exp, used:)
        _, name, superclass, *body = exp
        instructions = []
        if superclass
          instructions << transform_expression(superclass, used: true)
        else
          instructions << PushObjectClassInstruction.new
        end
        name, prep_instruction = constant_name(name)
        instructions << prep_instruction
        instructions << DefineClassInstruction.new(name: name)
        instructions += transform_body(body, used: true)
        instructions << EndInstruction.new(:define_class)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_colon2(exp, used:)
        _, namespace, name = exp
        instructions = [
          transform_expression(namespace, used: true),
          ConstFindInstruction.new(name, strict: true),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_colon3(exp, used:)
        return [] unless used
        _, name = exp
        [
          PushObjectClassInstruction.new,
          ConstFindInstruction.new(name, strict: true),
        ]
      end

      def transform_const(exp, used:)
        return [] unless used
        _, name = exp
        [
          PushSelfInstruction.new,
          ConstFindInstruction.new(name, strict: false),
        ]
      end

      def transform_cvar(exp, used:)
        return [] unless used
        _, name = exp
        ClassVariableGetInstruction.new(name)
      end

      def transform_cvdecl(exp, used:)
        _, name, value = exp
        instructions = [transform_expression(value, used: true), ClassVariableSetInstruction.new(name)]
        instructions << ClassVariableGetInstruction.new(name) if used
        instructions
      end

      def transform_def(exp, used:)
        _, name, args, *body = exp
        arity = Arity.new(args, is_proc: false).arity
        instructions = [
          DefineMethodInstruction.new(name: name, arity: arity),
          transform_defn_args(args, used: true),
          transform_body(body, used: true),
          EndInstruction.new(:define_method),
        ]
        instructions << PushSymbolInstruction.new(name) if used
        instructions
      end

      def transform_defined(exp, used:)
        return [] unless used
        _, name = exp
        body = transform_expression(name, used: true)
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

      def transform_defn(exp, used:)
        [
          PushSelfInstruction.new,
          transform_def(exp, used: used),
        ]
      end

      def transform_defn_args(exp, used:, for_block: false, check_args: true)
        return [] unless used
        _, *args = exp

        instructions = []

        if args.last.is_a?(Symbol) && args.last.start_with?('&')
          name = args.pop[1..]
          instructions << PushBlockInstruction.new
          instructions << VariableSetInstruction.new(name, local_only: true)
        end

        has_complicated_args = args.any? { |arg| arg.is_a?(Sexp) || arg.nil? || arg.start_with?('*') }
        may_need_to_destructure_args_for_block = for_block && args.size > 1

        if has_complicated_args || may_need_to_destructure_args_for_block
          min_count = minimum_arg_count(args)
          max_count = maximum_arg_count(args)

          if check_args
            instructions << CheckArgsInstruction.new(positional: min_count..max_count)
          end

          instructions << PushArgsInstruction.new(
            for_block: for_block,
            min_count: min_count,
            max_count: max_count,
            spread: for_block && args.size > 1,
          )

          instructions << Args.new(self, file: exp.file, line: exp.line).transform(exp.new(:args, *args))
          return instructions
        end

        if check_args
          instructions << CheckArgsInstruction.new(positional: args.size)
        end

        args.each_with_index do |name, index|
          instructions << PushArgInstruction.new(index, nil_default: for_block)
          instructions << VariableSetInstruction.new(name, local_only: true)
        end

        instructions
      end

      def transform_defs(exp, used:)
        _, owner, name, args, *body = exp
        [
          transform_expression(owner, used: true),
          SingletonClassInstruction.new,
          transform_def(exp.new(:defn, name, args, *body), used: used),
        ]
      end

      def transform_dot2(exp, used:, exclude_end: false)
        _, beginning, ending = exp
        instructions = [
          transform_expression(ending || s(:nil), used: true),
          transform_expression(beginning || s(:nil), used: true),
          PushRangeInstruction.new(exclude_end),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_dot3(exp, used:)
        transform_dot2(exp, used: used, exclude_end: true)
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
          PushArgcInstruction.new(0),
          transform_dstr(exp, used: true),
          SendInstruction.new(:to_sym, receiver_is_self: false, with_block: false, file: exp.file, line: exp.line),
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
        transform_rescue(
          exp.new(:rescue,
                  body,
                  exp.new(:resbody,
                          exp.new(:array),
                          exp.new(:block,
                                  ensure_body,
                                  exp.new(:call, nil, :raise))),
                  ensure_body),
          used: used,
        )
      end

      def transform_false(_, used:)
        return [] unless used
        PushFalseInstruction.new
      end

      def transform_gasgn(exp, used:)
        _, name, value = exp
        instructions = [transform_expression(value, used: true), GlobalVariableSetInstruction.new(name)]
        instructions << GlobalVariableGetInstruction.new(name) if used
        instructions
      end

      def transform_gvar(exp, used:)
        return [] unless used
        _, name = exp
        GlobalVariableGetInstruction.new(name)
      end

      def transform_hash(exp, used:, bare: false)
        _, *items = exp
        instructions = []
        if items.any? { |a| a.sexp_type == :kwsplat }
          instructions += transform_hash_with_kwsplat(items, bare: bare)
        else
          items.each do |item|
            instructions << transform_expression(item, used: true)
          end
          instructions << CreateHashInstruction.new(count: items.size / 2, bare: bare)
        end
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_hash_with_kwsplat(items, bare:)
        items = items.dup
        instructions = []

        # TODO: skip CreateHashInstruction if splat is first
        # (will need to duplicate the kwsplat value)

        # create hash from items before the splat
        prior_to_splat_count = 0
        while items.any? && items.first.sexp_type != :kwsplat
          instructions << transform_expression(items.shift, used: true)
          prior_to_splat_count += 1
        end
        instructions << CreateHashInstruction.new(count: prior_to_splat_count / 2, bare: bare)

        # now add to the hash the first splat item and everything after
        while items.any?
          key = items.shift
          if key.sexp_type == :kwsplat
            _, value = key
            instructions << transform_expression(value, used: true)
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

      def transform_iasgn(exp, used:)
        _, name, value = exp
        instructions = [transform_expression(value, used: true), InstanceVariableSetInstruction.new(name)]
        instructions << InstanceVariableGetInstruction.new(name) if used
        instructions
      end

      def transform_if(exp, used:)
        _, condition, true_expression, false_expression = exp
        true_instructions = transform_expression(true_expression || s(:nil), used: true)
        false_instructions = transform_expression(false_expression || s(:nil), used: true)
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
        instructions << transform_expression(body || s(:nil), used: true)
        instructions << EndInstruction.new(:define_block)
        case call.sexp_type
        when :call
          instructions << transform_call(call, used: used, with_block: true)
        when :lambda
          instructions << transform_lambda(call, used: used)
        when :super
          instructions << transform_super(call, used: used, with_block: true)
        when :zsuper
          instructions << transform_zsuper(call, used: used, with_block: true)
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

      def transform_lambda(_, used:)
        instructions = [CreateLambdaInstruction.new]
        instructions << PopInstruction.new unless used
        instructions
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
        when Symbol
          PushSymbolInstruction.new(lit)
        when Range
          [
            transform_lit(lit.end, used: true),
            transform_lit(lit.begin, used: true),
            PushRangeInstruction.new(lit.exclude_end?),
          ]
        when Regexp
          PushRegexpInstruction.new(lit)
        else
          raise "I don't yet know how to handle lit: #{lit.inspect}"
        end
      end

      def transform_lvar(exp, used:)
        return [] unless used
        _, name = exp
        VariableGetInstruction.new(name)
      end

      def transform_masgn(exp, used:)
        # s(:masgn,
        #   s(:array, s(:lasgn, :a), s(:lasgn, :b)),
        #   s(:to_ary, s(:array, s(:lit, 1), s(:lit, 2))))
        _, names_array, values = exp

        raise "Unexpected masgn names: #{names_array.inspect}" unless names_array.sexp_type == :array
        raise "Unexpected masgn values: #{values.inspect}" unless %i[array to_ary splat].include?(values.sexp_type)

        # We don't actually want to use a simple to_ary.
        # Our ToArrayInstruction is a little more special.
        values = values.last if values.sexp_type == :to_ary

        instructions = [
          transform_expression(values, used: true),
          DupInstruction.new,
          ToArrayInstruction.new,
          MultipleAssignment.new(self, file: exp.file, line: exp.line).transform(names_array),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_match2(exp, used:)
        _, regexp, string = exp
        transform_call(exp.new(:call, regexp, :=~, string), used: used)
      end

      def transform_match3(exp, used:)
        _, string, regexp = exp
        transform_call(exp.new(:call, regexp, :=~, string), used: used)
      end

      def transform_module(exp, used:)
        _, name, *body = exp
        instructions = []
        name, prep_instruction = constant_name(name)
        instructions << prep_instruction
        instructions << DefineModuleInstruction.new(name: name)
        instructions += transform_body(body, used: true)
        instructions << EndInstruction.new(:define_module)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_next(exp, used:) # rubocop:disable Lint/UnusedMethodArgument
        _, value = exp
        value ||= s(:nil)
        [
          transform_expression(value, used: true),
          NextInstruction.new,
        ]
      end

      def transform_nil(_, used:)
        return [] unless used
        PushNilInstruction.new
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

      def transform_op_asgn_and(exp, used:)
        _, variable, assignment = exp
        var_instruction = if variable.sexp_type == :lvar
                            _, name = variable
                            VariableGetInstruction.new(name, default_to_nil: true)
                          else
                            transform_expression(variable, used: true)
                          end
        instructions = [
          var_instruction,
          IfInstruction.new,
          transform_expression(assignment, used: true),
          ElseInstruction.new(:if),
          var_instruction,
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_op_asgn_or(exp, used:)
        _, variable, assignment = exp
        var_instruction = case variable.sexp_type
                          when :lvar
                            _, name = variable
                            VariableGetInstruction.new(name, default_to_nil: true)
                          when :cvar
                            _, name = variable
                            ClassVariableGetInstruction.new(name, default_to_nil: true)
                          else
                            transform_expression(variable, used: true)
                          end
        instructions = [
          var_instruction,
          IfInstruction.new,
          var_instruction,
          ElseInstruction.new(:if),
          transform_expression(assignment, used: true),
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_op_asgn1(exp, used:)
        _, obj, (_, *key_args), op, value = exp
        if op == :'||'
          instructions = [
            key_args.map { |arg| transform_expression(arg, used: true) },
            key_args.each_with_index.map { |_, index| DupRelInstruction.new(index) }, # key(s) are reused when the value is set
            PushArgcInstruction.new(key_args.size),
            transform_expression(obj, used: true),
            SendInstruction.new(:[], receiver_is_self: false, with_block: false, file: exp.file, line: exp.line),
            DupInstruction.new,
            IfInstruction.new,
            key_args.map { PopInstruction.new }, # didn't need the extra key(s) after all :-)
            ElseInstruction.new(:if),
            PopInstruction.new,
            transform_expression(value, used: true),
            PushArgcInstruction.new(key_args.size + 1),
            transform_expression(obj, used: true),
            SendInstruction.new(:[]=, receiver_is_self: false, with_block: false, file: exp.file, line: exp.line),
            EndInstruction.new(:if),
          ]
        else
          instructions = [
            # old_value = obj[key]
            transform_expression(obj, used: true),
            key_args.map { |arg| transform_expression(arg, used: true) },
            key_args.each_with_index.map { |_, index| DupRelInstruction.new(index) }, # key(s) are reused when the value is set
            PushArgcInstruction.new(key_args.size),
            DupRelInstruction.new(key_args.size * 2 + 1),
            SendInstruction.new(:[], receiver_is_self: false, with_block: false, file: exp.file, line: exp.line),
            # new_value = old_value + value
            transform_expression(value, used: true),
            PushArgcInstruction.new(1),
            MoveRelInstruction.new(2),
            SendInstruction.new(op, receiver_is_self: false, with_block: false, file: exp.file, line: exp.line),
            # obj[key] = new_value
            PushArgcInstruction.new(key_args.size + 1),
            MoveRelInstruction.new(key_args.size + 2),
            SendInstruction.new(:[]=, receiver_is_self: false, with_block: false, file: exp.file, line: exp.line),
          ]
          # a[b] += 1
          #
          # 0 push_argc 0
          # 1 push_self
          # 2 send :a to self        [a]
          # 3 dup                    [a, a]
          # 4 push_argc 0
          # 5 push_self
          # 6 send :b to self        [a, a, b]
          # 7 dup_rel 0              [a, a, b, b]
          # 8 push_argc 1
          # 9 push_argc 0
          # 10 push_self
          # 11 send :a to self
          # 12 send :[]
          # 13 push_int 1
          # 14 swap
          # 15 push_argc 1
          # 16 swap
          # 17 send :+
          # 18 push_argc 2
          # 19 swap
          # 20 send :[]=
        end
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_op_asgn2(exp, used:)
        _, obj, writer, op, *val_args = exp
        raise 'expected writer=' unless writer =~ /=$/
        reader = writer.to_s.chop
        if op == :'||'
          raise 'todo'
        else
          # given: obj.foo += 2
          instructions = [
            # obj
            transform_expression(obj, used: true),

            # 2
            val_args.map { |arg| transform_expression(arg, used: true) },

            # temp = obj.foo
            PushArgcInstruction.new(0),
            DupRelInstruction.new(2),
            SendInstruction.new(reader, receiver_is_self: false, with_block: false, file: exp.file, line: exp.line),

            # result = temp + 2
            PushArgcInstruction.new(val_args.size),
            SwapInstruction.new,
            SendInstruction.new(op, receiver_is_self: false, with_block: false, file: exp.file, line: exp.line),

            # obj.foo = result
            PushArgcInstruction.new(1),
            DupRelInstruction.new(2),
            SendInstruction.new(writer, receiver_is_self: false, with_block: false, file: exp.file, line: exp.line),
          ]
          instructions << PopInstruction.new unless used
          instructions
        end
      end

      def transform_or(exp, used:)
        _, lhs, rhs = exp
        lhs_instructions = transform_expression(lhs, used: true)
        rhs_instructions = transform_expression(rhs, used: true)
        instructions = [
          lhs_instructions,
          DupInstruction.new,
          IfInstruction.new,
          ElseInstruction.new(:if),
          PopInstruction.new,
          rhs_instructions,
          EndInstruction.new(:if),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_redo(*)
        [RedoInstruction.new]
      end

      def transform_rescue(exp, used:)
        instructions = Rescue.new(self).transform(exp)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_retry(*)
        [RetryInstruction.new(id: @retry_context.last)]
      end

      def transform_return(exp, used:)
        _, value = exp
        value ||= s(:nil)
        instructions = [transform_expression(value, used: true)]
        instructions << ReturnInstruction.new
      end

      def transform_safe_call(exp, used:)
        _, receiver, message, *args = exp

        call_args = transform_call_args(args)
        instructions = call_args.fetch(:instructions)

        instructions << transform_expression(receiver, used: true)
        instructions << DupInstruction.new
        instructions << IsNilInstruction.new
        instructions << IfInstruction.new
        instructions << PushNilInstruction.new
        instructions << ElseInstruction.new(:if)
        instructions << SendInstruction.new(
          message,
          args_array_on_stack: call_args.fetch(:args_array_on_stack),
          receiver_is_self: false,
          with_block: call_args.fetch(:with_block_pass),
          file: exp.file,
          line: exp.line,
        )
        instructions << EndInstruction.new(:if)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_sclass(exp, used:)
        _, owner, *body = exp
        instructions = [
          transform_expression(owner, used: true),
          SingletonClassInstruction.new,
          WithSelfInstruction.new,
          transform_body(body, used: true),
          EndInstruction.new(:with_self),
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_self(_, used:)
        return [] unless used
        PushSelfInstruction.new
      end

      def transform_splat(exp, used:)
        _, value = exp
        transform_expression(value, used: used)
      end

      def transform_str(exp, used:)
        return [] unless used
        _, str = exp
        PushStringInstruction.new(str)
      end

      def transform_super(exp, used:, with_block: false)
        _, *args = exp
        call_args = transform_call_args(args)
        instructions = call_args.fetch(:instructions)
        instructions << PushSelfInstruction.new
        instructions << SuperInstruction.new(
          args_array_on_stack: call_args.fetch(:args_array_on_stack),
          with_block: with_block || call_args.fetch(:with_block_pass),
        )
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_svalue(exp, used:)
        _, svalue = exp
        instructions = []
        case svalue.sexp_type
        when :splat
          instructions << transform_expression(svalue, used: true)
          instructions << ArrayWrapInstruction.new
        when :array
          instructions << transform_array(svalue, used: true)
        else
          raise "unexpected svalue type: #{svalue.inspect}"
        end
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_to_ary(exp, used:)
        _, value = exp
        instructions = [PushArgcInstruction.new(0)]
        instructions << transform_expression(value, used: true)
        instructions << SendInstruction.new(:to_ary, receiver_is_self: false, with_block: false, file: exp.file, line: exp.line)
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_true(_, used:)
        return [] unless used
        PushTrueInstruction.new
      end

      def transform_undef(exp, used:)
        _, name = exp
        name = name.last
        instructions = [
          UndefineMethodInstruction.new(name: name)
        ]
        instructions << PushNilInstruction.new if used
        instructions
      end

      def transform_until(exp, used:)
        _, condition, body, pre = exp
        transform_while(exp.new(:while, s(:call, condition, :!), body, pre), used: used)
      end

      def transform_while(exp, used:)
        _, condition, body, pre = exp
        body ||= s(:nil)

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

      def transform_xstr(exp, used:)
        _, command = exp
        instructions = [
          transform_str(exp.new(:str, command), used: true),
          ShellInstruction.new,
        ]
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_yield(exp, used:)
        _, *args = exp
        call_args = transform_call_args(args)
        instructions = call_args.fetch(:instructions)
        instructions << YieldInstruction.new(
          args_array_on_stack: call_args.fetch(:args_array_on_stack),
        )
        instructions << PopInstruction.new unless used
        instructions
      end

      def transform_zsuper(exp, used:, with_block: false)
        _, *args = exp
        instructions = []
        instructions << PushArgsInstruction.new(for_block: false, min_count: 0, max_count: 0)
        instructions << PushSelfInstruction.new
        instructions << SuperInstruction.new(
          args_array_on_stack: true,
          with_block: with_block,
        )
        instructions << PopInstruction.new unless used
        instructions
      end

      # HELPERS = = = = = = = = = = = = =

      # returns a pair of [name, prep_instruction]
      # prep_instruction being the instruction(s) needed to get the owner of the constant
      def constant_name(name)
        prepper = ConstPrepper.new(name, pass: self)
        [prepper.name, prepper.namespace]
      end

      def is_lambda_call?(exp)
        exp == s(:lambda) || exp == s(:call, nil, :lambda)
      end

      def minimum_arg_count(args)
        args.count do |arg|
          (arg.is_a?(Symbol) && arg !~ /^&|^\*/) ||
            (arg.is_a?(Sexp) && arg.sexp_type == :masgn)
        end
      end

      def maximum_arg_count(args)
        if args.any? { |arg| arg.is_a?(Symbol) && arg.start_with?('*') && !arg.start_with?('**') }
          # splat, no maximum
          return nil
        end

        args.count do |arg|
          !arg.is_a?(Symbol) || !arg.start_with?('&')
        end
      end

      def repl?
        @compiler_context[:repl]
      end

      # HACK: When using the REPL, the parser doesn't differentiate between method calls
      # and variable lookup. Both cases look like this: `s(:call, nil, :foo)`.
      # But we know which variables are defined in the REPL, so we can convert the :call
      # back to an :lvar as needed.
      # TODO: A better alternative would be to pass the variable names into NatalieParser
      # so that it can know which variables are defined already and return s(:lvar, :foo).
      def fix_repl_var_that_looks_like_call(exp)
        unless exp.size == 3 && exp[..1] == [:call, nil]
          return false
        end

        name = exp.last
        unless @compiler_context[:vars].key?(name)
          return false
        end

        exp.new(:lvar, exp.last)
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
