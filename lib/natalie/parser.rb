$LOAD_PATH << File.expand_path('../../build/prism/lib', __dir__)
$LOAD_PATH << File.expand_path('../../build/prism/ext', __dir__)
require 'prism'

module Natalie
  class Parser
    class IncompleteExpression < StandardError
    end

    # We use BasicVisitor here because it will give us a helpful error if one of the
    # visit_* methods are not defined.
    class SexpVisitor < ::Prism::BasicVisitor
      def initialize(path)
        super()
        @path = path
      end

      def visit_passthrough(node)
        node
      end

      def visit_alias_method_node(node)
        s(:alias, visit(node.new_name), visit(node.old_name), location: node.location)
      end

      def visit_and_node(node)
        s(:and, visit(node.left), visit(node.right), location: node.location)
      end

      def visit_array_node(node)
        s(:array, *node.child_nodes.map { |n| visit(n) }, location: node.location)
      end

      def visit_assoc_node(node)
        [visit(node.key), visit(node.value)]
      end

      def visit_assoc_splat_node(node)
        s(:kwsplat, visit(node.value), location: node.location)
      end

      def visit_back_reference_read_node(node)
        name = node.slice[1..].to_sym
        case name
        when :"'", :`
          s(:gvar, :"$#{name}", location: node.location)
        else
          s(:back_ref, name, location: node.location)
        end
      end

      def visit_begin_node(node)
        if !node.rescue_clause && !node.else_clause
          if node.ensure_clause
            s(:ensure,
              visit(node.statements),
              visit(node.ensure_clause.statements),
              location: node.location)
          else
            visit(node.statements)
          end
        else
          res = s(:rescue, location: node.location)
          res << visit(node.statements) if node.statements
          rescue_clause = node.rescue_clause
          res << visit(rescue_clause)
          while (rescue_clause = rescue_clause.consequent)
            res << visit(rescue_clause)
          end
          res << visit(node.else_clause) if node.else_clause
          if node.ensure_clause
            s(:ensure,
              res,
              visit(node.ensure_clause.statements),
              location: node.location)
          else
            res
          end
        end
      end

      def visit_block_argument_node(node)
        s(:block_pass, visit(node.expression), location: node.location)
      end

      def visit_block_node(node, call:)
        raise "unexpected node: #{node.inspect}" unless node.is_a?(Prism::BlockNode) || node.is_a?(Prism::LambdaNode)

        s(:iter,
          call,
          visit(node.parameters) || 0,
          visit(node.body),
          location: node.location)
      end

      def visit_block_parameter_node(node)
        "&#{node.name}".to_sym
      end

      def visit_block_parameters_node(node)
        visit_parameters_node(node.parameters)
      end

      def visit_break_node(node)
        visit_return_or_next_or_break_node(node, sexp_type: :break)
      end

      def visit_call_node(node)
        if %w[=~ !~].include?(node.name) &&
          [node.receiver, node.arguments&.child_nodes&.first].any? { |n| n.is_a?(Prism::RegularExpressionNode) }
          return visit_match_node(node)
        end

        sexp_type = if node.safe_navigation?
                      :safe_call
                    else
                      :call
                    end

        args, block = node_arguments_and_block(node)

        call = s(sexp_type,
                visit(node.child_nodes.first),
                node.name.to_sym,
                *args,
                location: node.location)

        if call[2] == :!~
          call[2] = :=~
          call = s(:not, call, location: node.location)
        end

        if block
          visit_block_node(block, call: call)
        else
          call
        end
      end

      def visit_call_operator_or_write_node(node)
        if node.target.name.to_sym == :[]=
          args = node.target.arguments&.child_nodes || []
          s(:op_asgn1,
            visit(node.target.receiver),
            s(:arglist, *args.map { |n| visit(n) }, location: node.location),
            node.operator.tr('=', '').to_sym,
            visit(node.value),
            location: node.location)
        elsif node.operator.to_sym == :'||='
          receiver = visit(node.target.receiver)
          s(:op_asgn_or,
            s(:call,
              receiver,
              node.target.name.tr('=', '').to_sym,
              location: node.target.location),
            s(:attrasgn,
              receiver,
              node.target.name.to_sym,
              visit(node.value),
              location: node.value.location),
            location: node.location)
        else
          s(:op_asgn2,
            visit(node.target.receiver),
            node.target.name.to_sym,
            node.operator.tr('=', '').to_sym,
            visit(node.value),
            location: node.location)
        end
      end

      def visit_call_or_write_node(node)
        if node.read_name.to_sym == :[]
          args = node.arguments&.child_nodes || []
          s(:op_asgn1,
            visit(node.receiver),
            s(:arglist, *args.map { |n| visit(n) }, location: node.location),
            node.operator.tr('=', '').to_sym,
            visit(node.value),
            location: node.location)
        else
          receiver = visit(node.receiver)
          s(:op_asgn_or,
            s(:call,
              receiver,
              node.read_name.to_sym,
              location: node.receiver.location),
            s(:attrasgn,
              receiver,
              node.write_name.to_sym,
              visit(node.value),
              location: node.value.location),
            location: node.location)
        end
      end

      def visit_case_node(node)
        raise SyntaxError, 'expected at least one when clause for case' if node.conditions.empty?

        s(:case,
          visit(node.predicate),
          *node.conditions.map { |n| visit(n) },
          visit(node.consequent),
          location: node.location)
      end

      def visit_class_node(node)
        name = visit(node.constant_path)
        name = name[1] if name.sexp_type == :const
        s(:class,
          name,
          visit(node.superclass),
          visit(node.body),
          location: node.location)
      end

      def visit_class_variable_read_node(node)
        s(:cvar, node.name, location: node.location)
      end

      def visit_class_variable_or_write_node(node)
        s(:op_asgn_or,
          s(:cvar, node.name, location: node.location),
          s(:cvdecl, node.name, visit(node.value), location: node.location),
          location: node.location)
      end

      def visit_class_variable_write_node(node)
        s(:cvdecl, node.name, visit(node.value), location: node.location)
      end

      def visit_class_variable_operator_write_node(node)
        visit_operator_write_node(node, read_sexp_type: :cvar, write_sexp_type: :cvdecl)
      end

      def visit_constant_path_node(node)
        if node.parent
          s(:colon2,
            visit(node.parent),
            node.child.slice.to_sym,
            location: node.location)
        else
          s(:colon3, node.child.slice.to_sym, location: node.location)
        end
      end

      def visit_constant_path_write_node(node)
        s(:cdecl,
          visit(node.target),
          visit(node.value),
          location: node.location)
      end

      def visit_constant_read_node(node)
        name = node.slice.to_sym
        s(:const, name, location: node.location)
      end

      def visit_constant_target_node(node)
        s(:cdecl, node.slice.to_sym, location: node.location)
      end

      def visit_constant_write_node(node)
        s(:cdecl, node.name.to_sym, visit(node.value), location: node.location)
      end

      def visit_call_operator_write_node(node)
        if node.write_name.to_sym == :[]=
          args = node.arguments&.child_nodes || []
          s(:op_asgn1,
            visit(node.receiver),
            s(:arglist, *args.map { |n| visit(n) }, location: node.location),
            node.operator,
            visit(node.value),
            location: node.location)
        else
          s(:op_asgn2,
            visit(node.receiver),
            node.write_name.to_sym,
            node.operator,
            visit(node.value),
            location: node.location)
        end
      end

      def visit_def_node(node)
        if node.receiver
          receiver = visit(node.receiver)
          if receiver.sexp_type == :call
            # NOTE: possible bug? https://github.com/ruby/prism/issues/1435
            receiver = s(:lvar, receiver.last, location: node.receiver.location)
          end
          s(:defs,
            receiver,
            node.name.to_sym,
            visit(node.parameters) || s(:args, location: node.location),
            visit(node.body) || s(:nil, location: node.location),
            location: node.location)
        else
          s(:defn,
            node.name.to_sym,
            visit(node.parameters) || s(:args, location: node.location),
            visit(node.body) || s(:nil, location: node.location),
            location: node.location)
        end
      end

      def visit_defined_node(node)
        s(:defined, visit(node.value), location: node.location)
      end

      def visit_else_node(node)
        visit(node.child_nodes.first)
      end

      def visit_false_node(node)
        s(:false, location: node.location)
      end

      def visit_float_node(node)
        s(:lit, node.value, location: node.location)
      end

      def visit_for_node(node)
        s(:for,
          visit(node.collection),
          visit(node.index),
          visit(node.statements),
          location: node.location)
      end

      def visit_forwarding_arguments_node(node)
        s(:forward_args, location: node.location)
      end

      def visit_forwarding_parameter_node(node)
        s(:forward_args, location: node.location)
      end

      def visit_forwarding_super_node(node)
        call = s(:zsuper, location: node.location)
        if node.block
          visit_block_node(node.block, call: call)
        else
          call
        end
      end

      def visit_global_variable_and_write_node(node)
        s(:op_asgn_and,
          s(:gvar, node.name, location: node.location),
          s(:gasgn, node.name, visit(node.value), location: node.location),
          location: node.location)
      end

      def visit_global_variable_operator_write_node(node)
        visit_operator_write_node(node, read_sexp_type: :gvar, write_sexp_type: :gasgn)
      end

      def visit_global_variable_or_write_node(node)
        s(:op_asgn_or,
          s(:gvar, node.name, location: node.location),
          s(:gasgn, node.name, visit(node.value), location: node.location),
          location: node.location)
      end

      def visit_global_variable_read_node(node)
        s(:gvar, node.name, location: node.location)
      end

      def visit_global_variable_target_node(node)
        s(:gasgn, node.name, location: node.location)
      end

      def visit_global_variable_write_node(node)
        s(:gasgn, node.name, visit(node.value), location: node.location)
      end

      def visit_hash_node(node)
        s(:hash,
          *flatten(node.child_nodes.map { |n| visit(n) }),
          location: node.location)
      end

      def visit_if_node(node)
        s(:if,
          visit(node.predicate),
          visit(node.statements),
          visit(node.consequent),
          location: node.location)
      end

      def visit_imaginary_node(node)
        s(:lit, node.value, location: node.location)
      end

      def visit_interpolated_regular_expression_node(node)
        dregx = visit_interpolated_stringish_node(node, sexp_type: :dregx, unescaped: false)
        dregx << node.options if node.options != 0
        dregx
      end

      def visit_instance_variable_and_write_node(node)
        s(:op_asgn_and,
          s(:ivar, node.name, location: node.location),
          s(:iasgn, node.name, visit(node.value), location: node.location),
          location: node.location)
      end

      def visit_instance_variable_operator_write_node(node)
        visit_operator_write_node(node, read_sexp_type: :ivar, write_sexp_type: :iasgn)
      end

      def visit_instance_variable_or_write_node(node)
        s(:op_asgn_or,
          s(:ivar, node.name, location: node.location),
          s(:iasgn, node.name, visit(node.value), location: node.location),
          location: node.location)
      end

      def visit_instance_variable_read_node(node)
        s(:ivar, node.name, location: node.location)
      end

      def visit_instance_variable_target_node(node)
        s(:iasgn, node.name, location: node.location)
      end

      def visit_instance_variable_write_node(node)
        s(:iasgn, node.name, visit(node.value), location: node.location)
      end

      alias visit_integer_node visit_passthrough

      def visit_interpolated_string_node(node)
        visit_interpolated_stringish_node(node, sexp_type: :dstr)
      end

      def visit_interpolated_stringish_node(node, sexp_type:, unescaped: true)
        segments = node.parts.map do |n|
                    case n
                    when Prism::StringNode
                      s(:str, unescaped ? n.unescaped : n.content, location: n.location)
                    when Prism::EmbeddedStatementsNode
                      raise 'unexpected evstr size' if n.statements.body.size != 1
                      s(:evstr, visit(n.statements.body.first), location: n.location)
                    else
                      raise "unknown interpolated string segment: #{n.inspect}"
                    end
                  end
        if (simpler = extract_single_segment_from_interpolated_node(segments, node: node, sexp_type: sexp_type))
          return simpler
        end
        start = if segments.first.sexp_type == :str
                  segments.shift[1]
                else
                  ''
                end
        s(sexp_type, start, *segments, location: node.location)
      end

      def extract_single_segment_from_interpolated_node(segments, node:, sexp_type:)
        return unless segments.size == 1

        segment = segments.first
        segment = if segment.sexp_type == :str
                    segment
                  elsif segment.sexp_type == :evstr && segment[1].is_a?(Sexp) && segment[1].sexp_type == :str
                    segment[1]
                  end
        return unless segment

        if sexp_type == :dstr
          segment
        else
          s(sexp_type, segment[1], location: node.location)
        end
      end

      def visit_interpolated_symbol_node(node)
        visit_interpolated_stringish_node(node, sexp_type: :dsym)
      end

      def visit_interpolated_x_string_node(node)
        visit_interpolated_stringish_node(node, sexp_type: :dxstr)
      end

      def visit_keyword_hash_node(node)
        s(:bare_hash,
          *flatten(node.child_nodes.map { |n| visit(n) }),
          location: node.location)
      end

      def visit_keyword_parameter_node(node)
        exp = s(:kwarg, node.name, location: node.location)
        exp << visit(node.value) if node.value
        exp
      end

      def visit_keyword_rest_parameter_node(node)
        "**#{node.name}".to_sym
      end

      def visit_lambda_node(node)
        visit_block_node(
          node,
          call: s(:lambda, location: node.location)
        )
      end

      def visit_local_variable_and_write_node(node)
        s(:op_asgn_and,
          s(:lvar,
            node.name,
            location: node.location),
          s(:lasgn,
            node.name,
            visit(node.value),
            location: node.location),
          location: node.location)
      end

      def visit_local_variable_operator_write_node(node)
        visit_operator_write_node(node, read_sexp_type: :lvar, write_sexp_type: :lasgn)
      end

      def visit_local_variable_or_write_node(node)
        s(:op_asgn_or,
          s(:lvar, node.name, location: node.location),
          s(:lasgn, node.name, visit(node.value), location: node.location),
          location: node.location)
      end

      def visit_local_variable_read_node(node)
        s(:lvar, node.name, location: node.location)
      end

      def visit_local_variable_target_node(node)
        s(:lasgn, node.name, location: node.location)
      end

      def visit_local_variable_write_node(node)
        s(:lasgn, node.name, visit(node.value), location: node.location)
      end

      def visit_match_node(node)
        match = if node.receiver.is_a?(Prism::RegularExpressionNode)
                  s(:match2,
                    visit(node.receiver),
                    visit(node.arguments.child_nodes.first),
                    location: node.location)
                else
                  s(:match3,
                    visit(node.arguments.child_nodes.first),
                    visit(node.receiver),
                    location: node.location)
                end
        if node.name == '!~'
          s(:not, match, location: node.location)
        else
          match
        end
      end

      def visit_match_write_node(node)
        s(:match_write, visit(node.call), *node.locals, location: node.location)
      end

      def visit_missing_node(_)
        raise SyntaxError, 'syntax error'
      end

      def visit_module_node(node)
        name = visit(node.constant_path)
        name = name[1] if name.sexp_type == :const
        s(:module,
          name,
          visit(node.body),
          location: node.location)
      end

      def visit_multi_target_node(node)
        return visit(node.targets.first) if node.targets.size == 1

        s(:masgn,
          s(:array, *node.targets.map { |n| visit(n) }, location: node.location),
          location: node.location)
      end

      def visit_multi_write_node(node)
        return visit(node.targets.first) if node.targets.size == 1 && !node.value

        masgn = s(:masgn,
                  s(:array, *node.targets.map { |n| visit(n) }, location: node.location),
                  location: node.location)
        if node.value
          value = visit(node.value)
          unless value.sexp_type == :array
            value = s(:to_ary, value, location: node.location)
          end
          masgn << value
        end
        masgn
      end

      def visit_next_node(node)
        visit_return_or_next_or_break_node(node, sexp_type: :next)
      end

      def visit_nil_node(node)
        s(:nil, location: node.location)
      end

      def visit_numbered_reference_read_node(node)
        s(:nth_ref, node.number, location: node.location)
      end

      def visit_operator_write_node(node, read_sexp_type:, write_sexp_type:)
        s(write_sexp_type,
          node.name,
          s(:call,
            s(read_sexp_type, node.name, location: node.location),
            node.operator,
            visit(node.value),
            location: node.location),
          location: node.location)
      end

      def visit_optional_parameter_node(node)
        s(:lasgn, node.name, visit(node.value), location: node.location)
      end

      def visit_or_node(node)
        s(:or, visit(node.left), visit(node.right), location: node.location)
      end

      def visit_parameters_node(node)
        return Sexp.new(:args) unless node

        # NOTE: More info about sorted parameters: https://github.com/ruby/prism/issues/1436
        in_order = node.child_nodes.compact.sort_by { |n| n.location.start_offset }

        s(:args,
          *in_order.map { |n| visit(n) }.compact,
          location: node.location)
      end

      def visit_parentheses_node(node)
        if node.body
          visit(node.body)
        else
          s(:nil, location: node.location)
        end
      end

      def visit_program_node(node)
        visit(node.child_nodes.first)
      end

      def visit_range_node(node)
        if node.left.is_a?(Prism::IntegerNode) && node.right.is_a?(Prism::IntegerNode)
          left = node.left.value
          right = node.right.value
          if node.exclude_end?
            s(:lit, left...right, location: node.location)
          else
            s(:lit, left..right, location: node.location)
          end
        elsif node.exclude_end?
          s(:dot3, visit(node.left), visit(node.right), location: node.location)
        else
          s(:dot2, visit(node.left), visit(node.right), location: node.location)
        end
      end

      def visit_rational_node(node)
        s(:lit, node.value, location: node.location)
      end

      def visit_redo_node(node)
        s(:redo, location: node.location)
      end

      def visit_regular_expression_node(node)
        s(:lit,
          Regexp.new(node.content, node.options),
          location: node.location)
      end

      def visit_required_destructured_parameter_node(node)
        s(:masgn,
          *node.parameters.map { |n| visit(n) },
          location: node.location)
      end

      def visit_required_parameter_node(node)
        node.name.to_sym
      end

      def visit_rescue_modifier_node(node)
        s(:rescue,
          visit(node.expression),
          s(:resbody,
            s(:array, location: node.rescue_expression.location),
            visit(node.rescue_expression),
            location: node.rescue_expression.location),
          location: node.location)
      end

      def visit_rescue_node(node)
        ary = s(:array,
          *node.exceptions.map { |n| visit(n) },
          location: node.location)
        ref = visit(node.reference)
        if ref
          ref << s(:gvar, :$!, location: node.location)
          ary << ref
        end
        s(:resbody,
          ary,
          visit(node.statements),
          location: node.location)
      end

      def visit_rest_parameter_node(node)
        "*#{node.name}".to_sym
      end

      def visit_retry_node(node)
        s(:retry, location: node.location)
      end

      def visit_return_node(node)
        visit_return_or_next_or_break_node(node, sexp_type: :return)
      end

      def visit_return_or_next_or_break_node(node, sexp_type:)
        args = node.arguments&.child_nodes || []
        if args.empty?
          s(sexp_type, nil, location: node.location)
        elsif args.size == 1
          s(sexp_type, visit(args.first), location: node.location)
        else
          s(sexp_type,
            s(:array, *args.map { |n| visit(n) }, location: node.location),
            location: node.location)
        end
      end

      def visit_self_node(node)
        s(:self, location: node.location)
      end

      def visit_singleton_class_node(node)
        s(:sclass, visit(node.expression), visit(node.body), location: node.location)
      end

      def visit_source_line_node(node)
        s(:lit, node.location.start_line, location: node.location)
      end

      def visit_splat_node(node)
        s(:splat, visit(node.expression), location: node.location)
      end

      def visit_source_file_node(node)
        s(:str, node.filepath, location: node.location)
      end

      def visit_statements_node(node)
        s(:block, *node.child_nodes.map { |n| visit(n) }, location: node.location)
      end

      def visit_string_concat_node(node)
        left = visit(node.left)
        right = visit(node.right)
        case [left.sexp_type, right.sexp_type]
        when %i[str str]
          left[1] << right[1]
          left
        when %i[dstr dstr]
          right[1..].each do |segment|
            left << if segment.is_a?(String)
                      s(:str, segment, location: node.right.location)
                    else
                      segment
                    end
          end
          left
        when %i[str dstr]
          right[1] = left[1] + right[1]
          right
        when %i[dstr str]
          left << right
          left
        else
          raise SyntaxError, "Unexpected nodes for StringConcatNode: #{left.inspect} and #{right.inspect}"
        end
      end

      def visit_string_node(node)
        s(:str, node.unescaped, location: node.location)
      end

      def visit_super_node(node)
        args, block = node_arguments_and_block(node)
        call = s(:super, *args, location: node.location)
        if block
          visit_block_node(node.block, call: call)
        else
          call
        end
      end

      def visit_symbol_node(node)
        s(:lit, node.unescaped.to_sym, location: node.location)
      end

      def visit_true_node(node)
        s(:true, location: node.location)
      end

      def visit_undef_node(node)
        if node.names.size == 1
          s(:undef, visit(node.names.first), location: node.location)
        else
          s(:block,
            *node.names.map { |n| s(:undef, visit(n), location: n.location) },
            location: node.location)
        end
      end

      def visit_unless_node(node)
        s(:if,
          visit(node.predicate),
          visit(node.consequent),
          visit(node.statements),
          location: node.location)
      end

      def visit_until_node(node)
        s(:until,
          visit(node.predicate),
          visit(node.statements),
          !node.begin_modifier?,
          location: node.location)
      end

      def visit_when_node(node)
        s(:when,
          s(:array,
            *node.conditions.map { |n| visit(n) },
            location: node.location),
          visit(node.statements),
          location: node.location)
      end

      def visit_while_node(node)
        s(:while,
          visit(node.predicate),
          visit(node.statements),
          !node.begin_modifier?,
          location: node.location)
      end

      def visit_yield_node(node)
        arguments = node.arguments&.child_nodes || []
        s(:yield, *arguments.map { |n| visit(n) }, location: node.location)
      end

      def visit_x_string_node(node)
        s(:xstr, node.unescaped, location: node.location)
      end

      private

      def s(*items, location:)
        Sexp.new(*items, location: location, file: @path)
      end

      def flatten(ary)
        ary2 = []
        ary.map do |item|
          if item.instance_of?(Array)
            item.each do |i|
              ary2 << i
            end
          else
            ary2 << item
          end
        end
        ary2
      end

      def node_arguments_and_block(node)
        args = node.arguments&.arguments || []
        block = node.block
        if block.is_a?(Prism::BlockArgumentNode)
          args << block
          block = nil
        end
        args.map! { |n| visit(n) }
        [args, block]
      end
    end

    class Sexp < Array
      def initialize(*parts, location: nil, file: nil)
        super(parts.size)
        parts.each_with_index do |part, index|
          self[index] = part
        end
        self.file = file
        self.line = location&.start_line
        self.column = location&.start_column
      end

      attr_accessor :file, :line, :column

      def inspect
        "s(#{map(&:inspect).join(', ')})"
      end

      def pretty_print(q)
        q.group(1, 's(', ')') do
          q.seplist(self) { |v| q.pp(v) }
        end
      end

      def sexp_type
        first
      end

      def new(*parts)
        Sexp.new(*parts).tap do |sexp|
          sexp.file = file
          sexp.line = line
          sexp.column = column
        end
      end

      def strip_trailing_nils
        pop while last.nil?
        self
      end
    end

    def initialize(code_str, path)
      raise 'TODO' if RUBY_ENGINE == 'natalie'

      @code_str = code_str
      @path = path
    end

    def ast
      Prism
        .parse(@code_str, @path)
        .value
        .accept(SexpVisitor.new(@path))
    end
  end
end
