class SexpProcessor
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
    return if ast.nil? && !strict
    method = "process_#{ast.sexp_type}"
    context << ast.sexp_type
    if respond_to?(method)
      result = send(method, ast)
    elsif default_method
      result = send(default_method, ast)
    elsif strict
      raise NoMethodError, "undefined method `#{method}' for #{inspect}"
    else
      result = ast
    end
    context.pop
    result
  end
end

def s(*items)
  sexp = Parser::Sexp.new
  items.each do |item|
    sexp << item
  end
  sexp
end

Sexp = Parser::Sexp
