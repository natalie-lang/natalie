$LOAD_PATH << File.expand_path('../../build/prism/lib', __dir__)
$LOAD_PATH << File.expand_path('../../build/prism/ext', __dir__)
require 'prism'

module Prism
  class Node
    # This is to maintain the same interface as Sexp instances. It doesn't
    # provide the exact same thing, so as we migrate we'll need to update call
    # sites to handle the "correct" type.
    def sexp_type
      type
    end

    # Find this transformation method so that we can catch any places where we
    # might previously have been doing destructuring.
    def to_ary
      raise "Implicit destructuring not supported for prism nodes"
    end

    def location
      @location || Prism::Location.new(Source.new('unknown'), 0, 0)
    end

    def file
      location.path
    end

    def line
      location.start_line
    end
  end

  class Location
    # We need to store path information on each node.
    attr_accessor :path
  end

  # Create an ArrayNode with the optionally given elements and location.
  def self.array_node(elements: [], location: nil)
    ArrayNode.new(elements, nil, nil, location)
  end

  # Create a CallNode with the optionally given values.
  def self.call_node(receiver:, name:, arguments: [], block: nil, flags: 0, location: nil)
    arguments = ArgumentsNode.new(arguments, location)
    CallNode.new(receiver, nil, nil, nil, arguments, nil, block, flags, name, location)
  end

  # Create a ClassVariableWriteNode with the optionally given values.
  def self.class_variable_write_node(name:, value: nil, location: nil)
    ClassVariableWriteNode.new(name, nil, value, nil, location)
  end

  # Create a ConstantReadNode with the optionally given values.
  def self.constant_read_node(name:, location: nil)
    ConstantReadNode.new(name, location)
  end

  # Create a FalseNode with the optionally given location.
  def self.false_node(location: nil)
    FalseNode.new(location)
  end

  # Create an LocalVariableWriteNode with the optionally given location.
  def self.local_variable_write_node(name:, value:, location: nil)
    LocalVariableWriteNode.new(name, 0, nil, value, nil, location)
  end

  # Create a NilNode with the optionally given location.
  def self.nil_node(location: nil)
    NilNode.new(location)
  end

  # Create an OrNode with the optionally given left, right, and location.
  def self.or_node(left: nil, right: nil, location: nil)
    OrNode.new(left, right, nil, location)
  end

  # Create a StringNode with the optionally given location.
  def self.string_node(unescaped:, location: nil)
    StringNode.new(0, nil, nil, nil, unescaped, location)
  end

  # Create a TrueNode with the optionally given location.
  def self.true_node(location: nil)
    TrueNode.new(location)
  end
end

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
        node.location.path = @path
        node
      end

      def visit_alias_method_node(node)
        node.copy(new_name: visit(node.new_name), old_name: visit(node.old_name))
      end

      def visit_and_node(node)
        copy(node, left: visit(node.left), right: visit(node.right))
      end

      def visit_arguments_node(node)
        copy(node, arguments: node.arguments.map { |n| visit(n) })
      end

      def visit_array_node(node)
        copy(node, elements: node.elements.map { |element| visit(element) })
      end

      def visit_assoc_node(node)
        copy(node, key: visit(node.key), value: visit(node.value))
      end

      def visit_assoc_splat_node(node)
        copy(node, value: visit(node.value))
      end

      alias visit_back_reference_read_node visit_passthrough

      def visit_begin_node(node)
        copy(
          node,
          statements: visit(node.statements),
          rescue_clause: visit(node.rescue_clause),
          else_clause: visit(node.else_clause),
          ensure_clause: visit(node.ensure_clause)
        )
      end

      def visit_block_argument_node(node)
        copy(node, expression: visit(node.expression))
      end

      def visit_block_node(node)
        copy(node, parameters: visit(node.parameters), body: visit(node.body))
      end

      alias visit_block_parameter_node visit_passthrough

      def visit_block_parameters_node(node)
        visit_parameters_node(node.parameters) if node.parameters
      end

      def visit_break_node(node)
        copy(node, arguments: visit(node.arguments))
      end

      def visit_call_node(node)
        copy(
          node,
          receiver: visit(node.receiver),
          arguments: visit(node.arguments),
          block: visit(node.block)
        )
      end

      def visit_call_or_write_node(node)
        copy(
          node,
          receiver: visit(node.receiver),
          arguments: visit(node.arguments),
          value: visit(node.value)
        )
      end

      def visit_case_node(node)
        raise SyntaxError, 'expected at least one when clause for case' if node.conditions.empty?

        copy(
          node,
          predicate: visit(node.predicate),
          conditions: node.conditions.map { |n| visit(n) },
          consequent: visit(node.consequent)
        )
      end

      def visit_class_node(node)
        copy(
          node,
          constant_path: visit(node.constant_path),
          superclass: visit(node.superclass),
          body: visit(node.body)
        )
      end

      alias visit_class_variable_read_node visit_passthrough

      alias visit_class_variable_target_node visit_passthrough

      def visit_class_variable_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_class_variable_and_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_class_variable_operator_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_class_variable_or_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_constant_path_node(node)
        copy(node, parent: visit(node.parent), child: visit(node.child))
      end

      alias visit_constant_path_target_node visit_constant_path_node

      def visit_constant_path_write_node(node)
        copy(node, target: visit(node.target), value: visit(node.value))
      end

      alias visit_constant_read_node visit_passthrough

      alias visit_constant_target_node visit_passthrough

      def visit_constant_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_call_operator_write_node(node)
        copy(
          node,
          receiver: visit(node.receiver),
          arguments: visit(node.arguments),
          value: visit(node.value)
        )
      end

      def visit_def_node(node)
        copy(
          node,
          receiver: visit(node.receiver),
          parameters: visit(node.parameters),
          body: visit(node.body)
        )
      end

      def visit_defined_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_else_node(node)
        visit(node.child_nodes.first)
      end

      def visit_embedded_statements_node(node)
        copy(node, statements: visit(node.statements))
      end

      def visit_ensure_node(node)
        copy(node, statements: visit(node.statements))
      end

      alias visit_false_node visit_passthrough

      alias visit_float_node visit_passthrough

      def visit_for_node(node)
        copy(
          node,
          collection: visit(node.collection),
          index: visit(node.index),
          statements: visit(node.statements)
        )
      end

      alias visit_forwarding_arguments_node visit_passthrough

      alias visit_forwarding_parameter_node visit_passthrough

      def visit_forwarding_super_node(node)
        copy(
          node,
          block: visit(node.block)
        )
      end

      def visit_global_variable_and_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_global_variable_operator_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_global_variable_or_write_node(node)
        copy(node, value: visit(node.value))
      end

      alias visit_global_variable_read_node visit_passthrough

      alias visit_global_variable_target_node visit_passthrough

      def visit_global_variable_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_hash_node(node)
        copy(node, elements: node.elements.map { |n| visit(n) })
      end

      def visit_if_node(node)
        s(:if,
          visit(node.predicate),
          visit(node.statements),
          visit(node.consequent),
          location: node.location)
      end

      alias visit_imaginary_node visit_passthrough

      def visit_interpolated_regular_expression_node(node)
        copy(node, parts: node.parts.map { |n| visit(n) })
      end

      def visit_instance_variable_and_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_instance_variable_operator_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_instance_variable_or_write_node(node)
        copy(node, value: visit(node.value))
      end

      alias visit_instance_variable_read_node visit_passthrough

      alias visit_instance_variable_target_node visit_passthrough

      def visit_instance_variable_write_node(node)
        copy(node, value: visit(node.value))
      end

      alias visit_integer_node visit_passthrough

      def visit_interpolated_string_node(node)
        copy(node, parts: node.parts.map { |n| visit(n) })
      end

      def visit_interpolated_symbol_node(node)
        copy(node, parts: node.parts.map { |n| visit(n) })
      end

      def visit_interpolated_x_string_node(node)
        copy(node, parts: node.parts.map { |n| visit(n) })
      end

      def visit_keyword_hash_node(node)
        copy(node, elements: node.elements.map { |n| visit(n) })
      end

      def visit_keyword_parameter_node(node)
        copy(node, value: visit(node.value))
      end

      alias visit_keyword_rest_parameter_node visit_passthrough

      def visit_lambda_node(node)
        copy(node, parameters: visit(node.parameters), body: visit(node.body))
      end

      def visit_local_variable_and_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_local_variable_operator_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_local_variable_or_write_node(node)
        copy(node, value: visit(node.value))
      end

      alias visit_local_variable_read_node visit_passthrough

      alias visit_local_variable_target_node visit_passthrough

      def visit_local_variable_write_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_match_write_node(node)
        s(:match_write, visit(node.call), *node.locals, location: node.location)
      end

      def visit_missing_node(_)
        raise SyntaxError, 'syntax error'
      end

      def visit_module_node(node)
        copy(
          node,
          constant_path: visit(node.constant_path),
          body: visit(node.body)
        )
      end

      def visit_multi_target_node(node)
        return visit(node.targets.first) if node.targets.size == 1

        visit_passthrough(node)
      end

      def visit_multi_write_node(node)
        return visit(node.targets.first) if node.targets.size == 1 && !node.value

        copy(node, value: visit(node.value))
      end

      def visit_next_node(node)
        copy(node, arguments: visit(node.arguments))
      end

      alias visit_nil_node visit_passthrough

      alias visit_numbered_reference_read_node visit_passthrough

      def visit_optional_parameter_node(node)
        copy(node, value: visit(node.value))
      end

      def visit_or_node(node)
        copy(node, left: visit(node.left), right: visit(node.right))
      end

      def visit_parameters_node(node)
        copy(
          node,
          requireds: node.requireds.map { |n| visit(n) },
          rest: visit(node.rest),
          optionals: node.optionals.map { |n| visit(n) },
          posts: node.posts.map { |n| visit(n) },
          keywords: node.keywords.map { |n| visit(n) },
          keyword_rest: visit(node.keyword_rest),
          block: visit(node.block)
        )
      end

      def visit_parentheses_node(node)
        if node.body
          visit(node.body)
        else
          Prism.nil_node(location: node.location)
        end
      end

      def visit_program_node(node)
        visit(node.child_nodes.first)
      end

      def visit_range_node(node)
        copy(node, left: visit(node.left), right: visit(node.right))
      end

      alias visit_rational_node visit_passthrough

      alias visit_redo_node visit_passthrough

      alias visit_regular_expression_node visit_passthrough

      alias visit_required_destructured_parameter_node visit_passthrough

      alias visit_required_parameter_node visit_passthrough

      def visit_rescue_modifier_node(node)
        copy(
          node,
          expression: visit(node.expression),
          rescue_expression: visit(node.rescue_expression)
        )
      end

      def visit_rescue_node(node)
        copy(
          node,
          exceptions: node.exceptions.map { |exception| visit(exception) },
          reference: visit(node.reference),
          statements: visit(node.statements),
          consequent: visit(node.consequent)
        )
      end

      alias visit_rest_parameter_node visit_passthrough

      alias visit_retry_node visit_passthrough

      def visit_return_node(node)
        copy(node, arguments: visit(node.arguments))
      end

      alias visit_self_node visit_passthrough

      def visit_singleton_class_node(node)
        copy(node, expression: visit(node.expression), body: visit(node.body))
      end

      alias visit_source_line_node visit_passthrough

      def visit_splat_node(node)
        copy(node, expression: visit(node.expression))
      end

      alias visit_source_file_node visit_passthrough

      def visit_statements_node(node)
        copy(node, body: node.body.map { |n| visit(n) })
      end

      def visit_string_concat_node(node)
        left = visit(node.left)
        right = visit(node.right)
        case [left.type, right.type]
        when %i[string_node string_node]
          left.unescaped << right.unescaped
          left.content << right.content
          left
        when %i[interpolated_string_node interpolated_string_node]
          right.parts.each do |part|
            left.parts << part
          end
          left
        when %i[string_node interpolated_string_node]
          right.parts.unshift(left)
          right
        when %i[interpolated_string_node string_node]
          left.parts << right
          left
        else
          raise SyntaxError, "Unexpected nodes for StringConcatNode: #{left.inspect} and #{right.inspect}"
        end
      end

      alias visit_string_node visit_passthrough

      def visit_super_node(node)
        copy(
          node,
          arguments: visit(node.arguments),
          block: visit(node.block)
        )
      end

      alias visit_symbol_node visit_passthrough

      alias visit_true_node visit_passthrough

      def visit_undef_node(node)
        if node.names.size == 1
          s(:undef, visit(node.names.first), location: node.location)
        else
          ::Prism::StatementsNode.new(
            node.names.map { |n| s(:undef, visit(n), location: n.location) },
            node.location
          )
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
        copy(
          node,
          conditions: node.conditions.map { |n| visit(n) },
          statements: visit(node.statements)
        )
      end

      def visit_while_node(node)
        s(:while,
          visit(node.predicate),
          visit(node.statements),
          !node.begin_modifier?,
          location: node.location)
      end

      def visit_yield_node(node)
        copy(node, arguments: visit(node.arguments))
      end

      def visit_x_string_node(node)
        s(:xstr, node.unescaped, location: node.location)
      end

      private

      def copy(node, **kwargs)
        n = node.copy(**kwargs)
        n.location.path = @path
        n
      end

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

      def inspect(q = nil)
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

      def type
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

      def match(pattern, node = self)
        if node.is_a?(Sexp)
          node.each_with_index.all? do |part, index|
            pattern.nil? || match(pattern[index], part)
          end
        elsif pattern.instance_of?(Array)
          pattern.include?(node)
        elsif pattern.nil?
          true
        elsif node == pattern
          true
        else
          false
        end
      end
    end

    def initialize(code_str, path)
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
