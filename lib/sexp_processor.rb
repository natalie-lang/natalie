class SexpProcessor
  attr_accessor :default_method
  attr_accessor :warn_on_default
  attr_accessor :require_empty
  attr_accessor :strict
  attr_accessor :expected

  def initialize

  end

  def process(ast)
    method = "process_#{ast.sexp_type}"
    send(method, ast)
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
