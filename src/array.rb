class Array
  alias prepend unshift

  def |(other)
    union(other)
  end

  def permutation(len = nil)
    unless block_given?
      ary = self
      return enum_for(:permutation, len) { ary.permutation(len).entries.size }
    end

    len = len.truncate if len.is_a? Float
    len ||= size
    return if len > size

    if len == 0
      yield []
      return
    end

    perm = ->(ary, depth) do
      if ary.size > 1
        all = []
        ary.each_with_index do |start, index|
          rest = ary[0...index] + ary[index + 1..-1]
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
    end
    perm.(self, 0).each { |permutation| yield permutation }

    self
  end

  def repeated_permutation(len)
    len = len.truncate if len.is_a? Float
    unless block_given?
      ary = self
      return enum_for(:repeated_permutation, len) { len < 0 ? 0 : (ary.size**len) }
    end
    return if len < 0
    if len == 0
      yield []
      return
    end
    copy = clone
    copy.product(*(0..(len - 2)).entries.map { |_| copy }) { |combo| yield combo }
    self
  end

  def combination(num)
    unless block_given?
      accumulator = []
      combination(num) { |combo| accumulator << combo }
      return Enumerator.new(accumulator.size) { |yielder| accumulator.each { |item| yielder << item } }
    end
    if num == 0
      yield [] # One empty combination
    elsif num > size
      return self # There are no combinations, don't yield anything
    elsif num == 1
      each { |item| yield [item] }
      return
    elsif num > 0
      # A rough translation of combinate0 from MRI's ruby/array.c

      stack = [0] * (num + 1)
      stack[0] = -1

      lev = 0

      loop do
        lev += 1

        while lev < num
          stack[lev + 1] = stack[lev] + 1

          lev += 1
        end

        yield stack[1..-1].map { |i| self[i] }

        loop do
          return self if lev == 0

          stack[lev] = stack[lev] + 1
          lev -= 1

          break unless stack[lev + 1] + num == size + lev + 1
        end
      end
    end

    self
  end

  def repeated_combination(times)
    unless block_given?
      accumulator = []
      repeated_combination(times) { |combo| accumulator << combo }
      return Enumerator.new(accumulator.size) { |yielder| accumulator.each { |item| yielder << item } }
    end
    if times == 0
      yield []
    elsif times == 1
      each { |item| yield [item] }
      return
    elsif times > 0
      repeated_combination(times - 1) do |combo|
        (index(combo.last)..(size - 1)).each { |current| yield combo + [at(current)] }
      end
    end
    self
  end

  def sample(n = 1, random: Random::DEFAULT)
    n = n.to_int if !n.is_a?(Integer) && n.respond_to?(:to_int)
    raise ArgumentError, 'negative sample number' if n < 0
    array_size = size

    generate_index = -> do
      random
        .rand(array_size)
        .tap do |index|
          index = index.to_int if !index.is_a?(Integer) && index.respond_to?(:to_int)

          raise RangeError, "random number too small #{index}" if index < 0
          raise RangeError, "random number too big #{index}" if index >= array_size
        end
    end

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
      new_index = new_index.to_int if !new_index.is_a?(Integer) && new_index.respond_to?(:to_int)

      raise RangeError, "random number too small #{new_index}" if new_index < 0
      raise RangeError, "random number too big #{new_index}" if new_index >= index

      tmp = self[index]
      self[index] = self[new_index]
      self[new_index] = tmp
    end
    self
  end

  def transpose
    ary = []
    row_size = nil
    each_with_index do |row, index|
      row = row.to_ary if !row.is_a?(Array) && row.respond_to?(:to_ary)

      raise TypeError, "no implicit conversion of #{row.class.inspect} into Array" unless row.is_a?(Array)

      raise IndexError, "element size differ (#{row.size} should be #{row_size}" if row_size && row_size != row.size
      row_size = row.size

      row.each_with_index do |element, offset|
        ary[offset] ||= []
        ary[offset] << element
      end
    end
    ary
  end

  def union(*args)
    dup.concat(*args).uniq
  end
end
