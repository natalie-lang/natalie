class Array
  def permutation(len = size)
    if len == 0
      return [[]]
    elsif len > size
      return []
    end
    perm = ->(ary, depth) {
      if ary.size > 1
        all = []
        ary.each_with_index do |start, index|
          rest = ary[0...index] + ary[index+1..-1]
          if depth + 1 >= len
            all << [start]
          else
            all += perm.(rest, depth + 1).map { |rest| [start] + rest }
          end
        end
        all
      else
        [ary]
      end
    }
    perm.(self, 0)
  end
end
