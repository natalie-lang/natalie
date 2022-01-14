# some basic tests as we are starting out

class TestCompiler2
  def ary
    # we don't have array literal compilation yet, lol
    a = Array.new
    a << 1
    a << 2
    a << 3
    a
  end

  def run
    p ary.map { |i| i * 2 } # => [2, 4, 6]
  end
end

TestCompiler2.new.run
