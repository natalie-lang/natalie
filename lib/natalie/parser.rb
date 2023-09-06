$LOAD_PATH << File.expand_path('../../build/yarp/lib', __dir__)
$LOAD_PATH << File.expand_path('../../build/yarp/ext', __dir__)
require 'yarp'

class SexpVisitor < ::YARP::BasicVisitor
  def initialize(path)
    @path = path
  end

  def visit_call_node(node)
    arguments = node.child_nodes[1].child_nodes
    s(:call,
      visit(node.child_nodes.first),
      node.name.to_sym,
      *arguments.map { |n| visit(n) },
      location: node.location)
  end

  def visit_integer_node(node)
    s(:lit, node.value, location: node.location)
  end

  def visit_statements_node(node)
    s(:block, *node.child_nodes.map { |n| visit(n) }, location: node.location)
  end

  alias visit_program_node visit_statements_node

  def visit_alias_node(node)
    s(:alias, visit(node.new_name), visit(node.old_name), location: node.location)
  end

  private

  def s(*items, location: nil)
    Sexp.new(*items, location:, file: @path)
  end
end

class Sexp < Array
  def initialize(*parts, location: nil, file: nil)
    super(parts.size)
    parts.each_with_index do |part, index|
      self[index] = part
      self.file = file
      self.line = location.start_line
      self.column = location.start_column
    end
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
    Sexp.new(*self).tap do |sexp|
      sexp.file = file
      sexp.line = line
      sexp.column = column
    end
  end
end

module Natalie
  class Parser
    class IncompleteExpression < StandardError
    end

    def initialize(code_str, path, old_parser: false)
      raise 'TODO' if RUBY_ENGINE == 'natalie'

      @code_str = code_str
      @path = path
      @old_parser = old_parser
    end

    def ast
      if @old_parser
        $LOAD_PATH << File.expand_path('../../build/natalie_parser/lib', __dir__)
        $LOAD_PATH << File.expand_path('../../build/natalie_parser/ext', __dir__)
        require 'natalie_parser'
        begin
          result = NatalieParser.parse(@code_str, @path)
          if result.is_a?(Sexp) && result.sexp_type == :block
            result
          else
            result.new(:block, result)
          end
        rescue SyntaxError => e
          if e.message =~ /unexpected end-of-input/
            raise IncompleteExpression, e.message
          else
            raise
          end
        end
      else
        YARP.parse(@code_str, @path).value.accept(SexpVisitor.new(@path))
      end
    end
  end
end
