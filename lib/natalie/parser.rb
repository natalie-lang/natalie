$LOAD_PATH << File.expand_path('../../build/prism/lib', __dir__)
$LOAD_PATH << File.expand_path('../../build/prism/ext', __dir__)
require 'prism'

module Prism
  class Node
    # Find this transformation method so that we can catch any places where we
    # might previously have been doing destructuring.
    def to_ary
      raise 'Implicit destructuring not supported for prism nodes'
    end

    def location
      @location || Prism::Location.new(Source.new('unknown'), 0, 0)
    end
  end

  # Create an ArrayNode with the optionally given elements and location.
  def self.array_node(elements: [], location: nil)
    ArrayNode.new(0, elements, nil, nil, location)
  end

  # Create a CallNode with the optionally given values.
  def self.call_node(receiver:, name:, arguments: [], block: nil, flags: 0, location: nil)
    arguments = ArgumentsNode.new(0, arguments, location)
    CallNode.new(flags, receiver, nil, name, nil, nil, arguments, nil, block, location)
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
    class ParseError < StandardError
    end

    class IncompleteExpression < ParseError
    end

    def initialize(code_str, path, locals: [])
      @code_str = code_str
      @path = path
      @locals = locals
    end

    def result
      @result ||= Prism.parse(@code_str, filepath: @path, scopes: [@locals])
    end

    def errors
      result.errors
    end

    def ast
      raise ParseError, "syntax error: #{result.errors.map(&:message).join("\n")}" if result.errors.any?

      result.value
    end

    def encoding
      result.source.source.encoding
    end
  end
end
