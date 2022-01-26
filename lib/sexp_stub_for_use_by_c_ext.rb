# Natalie proper should not use this. It's only here because the Parser C-extension for MRI needs it.
class Sexp < Array
  def initialize(*items)
    items.each { |i| self << i }
  end

  def self.from_array(ary)
    Sexp.new(*ary)
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
end
