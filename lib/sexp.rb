class Sexp < Array
  def initialize(*items)
    items.each do |item|
      self << item
    end
  end

  def inspect
    's(' + map(&:inspect).join(', ') + ')'
  end
end

def s(*items)
  Sexp.new(*items)
end
