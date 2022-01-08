# TODO: rename to just SexpProcessor once the ruby_parser gem is gone.
class NatSexpProcessor
  attr_accessor :default_method
  attr_accessor :warn_on_default
  attr_accessor :require_empty
  attr_accessor :strict
  attr_accessor :expected

  attr_reader :context

  def initialize
    @context = []
  end

  def process(ast)
    return if ast.nil?
    method = "process_#{ast.sexp_type}"
    context << ast.sexp_type
    if respond_to?(method)
      result = send(method, ast)
    elsif default_method
      result = send(default_method, ast)
    elsif strict
      raise NoMethodError, "undefined method `#{method}' for #{inspect}"
    elsif ast.is_a?(Array)
      result = Sexp.new
      result << ast.sexp_type
      ast[1..-1].each { |item| item.is_a?(Array) ? result << process(item) : result << item }
      if ast.is_a?(Sexp)
        result.file = ast.file
        result.line = ast.line
      end
    else
      result = ast
    end
    context.pop
    result
  end
end

def s(*items)
  sexp = Sexp.new
  items.each { |item| sexp << item }
  sexp
end
