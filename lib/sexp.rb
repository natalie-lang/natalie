class Sexp < Array
  attr_accessor :file, :line, :column

  def initialize(*items)
    items.each { |i| self << i }
  end

  def new(*items)
    Sexp.new.tap do |sexp|
      sexp.file = file
      sexp.line = line
      items.each { |i| sexp << i }
    end
  end

  def inspect
    "s(#{map(&:inspect).join(', ')})"
  end

  alias sexp_type first
end

def s(*items)
  sexp = Sexp.new
  items.each do |item|
    sexp << item
  end
  sexp
end
