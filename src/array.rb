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

  def sample(n = 1, random: Random::DEFAULT)
    if !n.is_a?(Integer) && n.respond_to?(:to_int)
      n = n.to_int
    end
    raise ArgumentError, 'negative sample number' if n < 0
    array_size = size

    generate_index = ->() {
      random.rand(array_size).tap do |index|
        if !index.is_a?(Integer) && index.respond_to?(:to_int)
          index = index.to_int
        end

        raise RangeError, "random number too small #{index}" if index < 0
        raise RangeError, "random number too big #{index}" if index >= array_size
      end
    }

    if n == 1
      return if empty?

      self[generate_index.()]
    else
      return [] if empty?

      indexes = []
      [].tap do |out|
        while out.size != [n, size].min
          index = generate_index.()

          unless indexes.include?(index)
            out << self[index]
            indexes << index
          end
        end
      end
    end
  end

  def shuffle(random: Random::DEFAULT)
    dup.shuffle!(random: random)
  end

  def shuffle!(random: Random::DEFAULT)
    raise FrozenError, "can't modify frozen #{self.class.name}: #{inspect}" if frozen?

    array_size = size
    (array_size - 1).downto(1) do |index|
      new_index = random.rand(index)
      if !new_index.is_a?(Integer) && new_index.respond_to?(:to_int)
        new_index = new_index.to_int
      end

      raise RangeError, "random number too small #{new_index}" if new_index < 0
      raise RangeError, "random number too big #{new_index}" if new_index >= index

      tmp = self[index]
      self[index] = self[new_index]
      self[new_index] = tmp
    end
    self
  end
end
