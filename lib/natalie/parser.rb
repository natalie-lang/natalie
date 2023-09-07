$LOAD_PATH << File.expand_path('../../build/yarp/lib', __dir__)
$LOAD_PATH << File.expand_path('../../build/yarp/ext', __dir__)
require 'yarp'

class SexpVisitor < ::YARP::BasicVisitor
  def initialize(path)
    @path = path
  end

  def visit_alias_node(node)
    s(:alias, visit(node.new_name), visit(node.old_name), location: node.location)
  end

  def visit_and_node(node)
    s(:and, visit(node.left), visit(node.right), location: node.location)
  end

  def visit_array_node(node)
    s(:array, *node.child_nodes.map { |n| visit(n) }, location: node.location)
  end

  def visit_block_argument_node(node)
    s(:block_pass, visit(node.expression), location: node.location)
  end

  def visit_block_parameters_node(node)
    visit_parameters_node(node.parameters)
  end

  def visit_call_node(node)
    arguments = node.arguments&.child_nodes || []
    call = s(:call,
            visit(node.child_nodes.first),
            node.name.to_sym,
            *arguments.map { |n| visit(n) },
            location: node.location)
    if node.block
      s(:iter,
        call,
        visit(node.block.parameters) || 0,
        visit(node.block.body),
        location: node.location)
    else
      call
    end
  end

  def visit_class_node(node)
    s(:class,
      node.name.to_sym,
      visit(node.superclass),
      visit(node.child_nodes[2]),
      location: node.location)
  end

  def visit_constant_read_node(node)
    name = node.slice.to_sym
    s(:const, name, location: node.location)
  end

  def visit_constant_write_node(node)
    s(:cdecl, node.name.to_sym, visit(node.value), location: node.location)
  end

  def visit_def_node(node)
    _unknown, parameters_node, statements_node = node.child_nodes
    s(:defn,
      node.name,
      visit(parameters_node) || Sexp.new(:args),
      visit(statements_node),
      location: node.location)
  end

  def visit_else_node(node)
    visit(node.child_nodes.first)
  end

  def visit_false_node(node)
    s(:false, location: node.location)
  end

  def visit_global_variable_read_node(node)
    s(:gvar, node.name, location: node.location)
  end

  def visit_hash_node(node)
    s(:hash,
      *node.child_nodes.flat_map { |n| [visit(n.key), visit(n.value)] },
      location: node.location)
  end

  def visit_if_node(node)
    condition, true_body, false_body = node.child_nodes
    s(:if, visit(condition), visit(true_body), visit(false_body), location: node.location)
  end

  def visit_instance_variable_read_node(node)
    s(:ivar, node.name, location: node.location)
  end

  def visit_instance_variable_write_node(node)
    s(:iasgn, node.name, visit(node.value), location: node.location)
  end

  def visit_integer_node(node)
    s(:lit, node.value, location: node.location)
  end

  def visit_interpolated_string_node(node)
    s(:dstr,
      '',
      *node.child_nodes.map do |n|
        case n
        when YARP::StringNode
          s(:str, n.content, location: n.location)
        when YARP::EmbeddedStatementsNode
          s(:evstr, visit(n.statements), location: n.location)
        else
          raise "unknown interpolated string segment: #{n.inspect}"
        end
      end,
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

  def visit_multi_write_node(node)
    value = visit(node.value)
    unless value.sexp_type == :array
      value = s(:to_ary, value, location: node.location)
    end
    s(:masgn,
      s(:array, *node.targets.map { |n| visit(n) }, location: node.location),
      value,
      location: node.location)
  end

  def visit_next_node(node)
    visit_return_or_next_node(node, sexp_type: :next)
  end

  def visit_parameters_node(node)
    return Sexp.new(:args) unless node

    s(:args,
      *node.child_nodes.map { |n| visit(n) }.compact,
      location: node.location)
  end

  def visit_range_node(node)
    left = visit(node.left)
    right = visit(node.right)
    if node.exclude_end?
      s(:lit, left...right, location: node.location)
    else
      s(:lit, left..right, location: node.location)
    end
  end

  def visit_required_destructured_parameter_node(node)
    s(:masgn,
      *node.parameters.map { |n| visit(n) },
      location: node.location)
  end

  def visit_required_parameter_node(node)
    node.name.to_sym
  end

  def visit_return_node(node)
    visit_return_or_next_node(node, sexp_type: :return)
  end

  def visit_return_or_next_node(node, sexp_type:)
    args = node.arguments&.child_nodes || []
    if args.size == 0
      s(sexp_type, nil, location: node.location)
    elsif args.size == 1
      s(sexp_type, visit(args.first), location: node.location)
    else
      s(sexp_type,
        s(:array, *args.map { |n| visit(n) }, location: node.location),
        location: node.location)
    end
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
  alias visit_program_node visit_statements_node

  def visit_string_node(node)
    s(:str, node.content, location: node.location)
  end

  def visit_symbol_node(node)
    s(:lit, node.value.to_sym, location: node.location)
  end

  def visit_true_node(node)
    s(:true, location: node.location)
  end

  def visit_yield_node(node)
    arguments = node.arguments&.child_nodes || []
    s(:yield, *arguments.map { |n| visit(n) }, location: node.location)
  end

  private

  def s(*items, location:)
    Sexp.new(*items, location:, file: @path)
  end
end

class Sexp < Array
  def initialize(*parts, location: nil, file: nil)
    super(parts.size)
    parts.each_with_index do |part, index|
      self[index] = part
      self.file = file
      self.line = location&.start_line
      self.column = location&.start_column
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
