class Sexp < Array
  def initialize(*items)
    items.each { |i| self << i }
  end

  attr_accessor :file, :line

  def inspect
    "s(#{map(&:inspect).join(', ')})"
  end

  alias sexp_type first

  def new(*items)
    s = Sexp.new(*items)
    s.file = file
    s.line = line
    s
  end

  def pretty_print q
    nnd = ")"
    q.group(1, "s(", nnd) do
      q.seplist(self) { |v| q.pp v }
    end
  end
  
  def s(*args)
    res = Sexp.new(*args)
    res.line=line
    res.file=file
    res
  end
end
