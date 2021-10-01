class Integer
  def downto(n)
    # NATFIXME: Return enumerator if no block given. We cannot store n yet.

    (self - n + 1).times do |i|
      yield self - i
    end
  end
end
