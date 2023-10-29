$LOAD_PATH << File.expand_path('../../build/prism/lib', __dir__)
$LOAD_PATH << File.expand_path('../../build/prism/ext', __dir__)
require 'prism'

module Prism
  class Node
    # Find this transformation method so that we can catch any places where we
    # might previously have been doing destructuring.
    def to_ary
      raise "Implicit destructuring not supported for prism nodes"
    end

    def location
      @location || Prism::Location.new(Source.new('unknown'), 0, 0)
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
    # TODO: figrue out how to raise this when relevant
    class IncompleteExpression < StandardError
    end

    # For convenience, we attach the file path to every node.
    class PathVisitor < ::Prism::Visitor
      def initialize(path)
        super()
        @path = path
      end

      def visit(node)
        node.location.path = @path if node
        super
      end

      def visit_child_nodes(node)
        node.compact_child_nodes.each do |node|
          node.location.path = @path if node
          node&.accept(self)
        end
        node
      end

      Prism::Visitor.instance_methods.each do |name|
        next unless name.start_with?('visit_')
        next if name == :visit_child_nodes || name == :visit

        alias_method name, :visit_child_nodes
      end

      def visit_missing_node(node)
        raise SyntaxError, "syntax error #{node.location.path}##{node.location.start_line}"
      end

      def visit_case_node(node)
        raise SyntaxError, 'expected at least one when clause for case' if node.conditions.empty?

        visit_child_nodes(node)
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
        .statements
        .accept(PathVisitor.new(@path))
    end
  end
end
