class Array
  alias prepend unshift

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

  def repeated_combination(times)
    unless block_given?
      accumulator = []
      repeated_combination(times) { |combo| accumulator << combo }
      return Enumerator.new(accumulator.size) do |yielder| 
        accumulator.each { |item| yielder << item }
      end
    end
    if times == 0
      yield []
    elsif times == 1
      each { |item| yield [item] }
      return
    elsif times > 0
      repeated_combination(times - 1) do |combo|
        (index(combo.last)..(size - 1)).each do |current|
          yield combo + [at(current)]
        end
      end
    end
    self
  end
end
