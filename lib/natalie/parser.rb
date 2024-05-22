$LOAD_PATH.unshift(File.expand_path('../../build/prism/lib', __dir__))
$LOAD_PATH.unshift(File.expand_path('../../build/prism/ext', __dir__))
require 'prism'

module Prism
  # Create an ArrayNode with the optionally given elements and location.
  def self.array_node(location:, elements: [])
    ArrayNode.new(nil, 0, elements, nil, nil, location)
  end

  # Create a CallNode with the optionally given values.
  def self.call_node(receiver:, name:, location:, arguments: [], block: nil, flags: 0)
    arguments = ArgumentsNode.new(nil, 0, arguments, location)
    CallNode.new(nil, flags, receiver, nil, name, nil, nil, arguments, nil, block, location)
  end

  # Create a ClassVariableWriteNode with the optionally given values.
  def self.class_variable_write_node(name:, location:, value: nil)
    ClassVariableWriteNode.new(nil, name, nil, value, nil, location)
  end

  # Create a ConstantReadNode with the optionally given values.
  def self.constant_read_node(name:, location:)
    ConstantReadNode.new(nil, name, location)
  end

  # Create a FalseNode with the optionally given location.
  def self.false_node(location:)
    FalseNode.new(nil, location)
  end

  # Create an LocalVariableWriteNode with the optionally given location.
  def self.local_variable_write_node(name:, value:, location:)
    LocalVariableWriteNode.new(nil, name, 0, nil, value, nil, location)
  end

  # Create a NilNode with the optionally given location.
  def self.nil_node(location:)
    NilNode.new(nil, location)
  end

  # Create an OrNode with the optionally given left, right, and location.
  def self.or_node(location:, left: nil, right: nil)
    OrNode.new(nil, left, right, nil, location)
  end

  # Create a StringNode with the optionally given location.
  def self.string_node(unescaped:, location:)
    StringNode.new(nil, 0, nil, nil, nil, unescaped, location)
  end

  # Create a TrueNode with the optionally given location.
  def self.true_node(location:)
    TrueNode.new(nil, location)
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

    def tokenize
      Prism.lex(@code_str)
    end

    def result
      @result ||= Prism.parse(@code_str, filepath: @path, scopes: [@locals])
    end

    def source = result.source
    def errors = result.errors

    def ast
      raise ParseError, "syntax error: #{result.errors.map(&:message).join("\n")}" if result.errors.any?

      result.value
    end

    def encoding
      result.source.source.encoding
    end
  end
end
