require_relative '../spec_helper'

describe 'range' do
  it 'can be created with .. syntax' do
    r = 1..10
    r.should be_kind_of(Range)
    r.begin.should == 1
    r.end.should == 10
    r.exclude_end?.should == false
    r = 'a'..'z'
    r.should be_kind_of(Range)
    r.begin.should == 'a'
    r.end.should == 'z'
    r.exclude_end?.should == false
  end

  it 'can be created with ... syntax' do
    r = 1...10
    r.should be_kind_of(Range)
    r.begin.should == 1
    r.end.should == 10
    r.exclude_end?.should == true
    r = 'a'...'z'
    r.should be_kind_of(Range)
    r.begin.should == 'a'
    r.end.should == 'z'
    r.exclude_end?.should == true
  end

  it 'can be created with Range#new' do
    r = Range.new(1, 10)
    r.should be_kind_of(Range)
    r.begin.should == 1
    r.end.should == 10
    r.exclude_end?.should == false
    r = Range.new(1, 10, true)
    r.should be_kind_of(Range)
    r.begin.should == 1
    r.end.should == 10
    r.exclude_end?.should == true
    r = Range.new('a', 'z')
    r.should be_kind_of(Range)
    r.begin.should == 'a'
    r.end.should == 'z'
  end

  describe '#begin' do
    it 'returns the beginning' do
      r = 1..10
      r.begin.should == 1
    end
  end

  describe '#first' do
    it 'returns the beginning' do
      r = 1..10
      r.first.should == 1
    end
  end

  describe '#end' do
    it 'returns the ending' do
      r = 1..10
      r.end.should == 10
    end
  end

  describe '#last' do
    it 'returns the ending' do
      r = 1..10
      r.last.should == 10
    end
  end

  describe '#exclude_end?' do
    it 'returns true if the ending is excluded' do
      r = 1...10
      r.exclude_end?.should == true
      r = Range.new(1, 10, true)
      r.exclude_end?.should == true
      r = Range.new(1, 10, Object.new)
      r.exclude_end?.should == true
    end

    it 'returns false if the ending is not excluded' do
      r = 1..10
      r.exclude_end?.should == false
      r = Range.new(1, 10)
      r.exclude_end?.should == false
      r = Range.new(1, 10, false)
      r.exclude_end?.should == false
    end
  end

  describe '#to_a' do
    it 'returns the range expaneded to an array' do
      r = 1..10
      r.to_a.should == [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
      r = 1...10
      r.to_a.should == [1, 2, 3, 4, 5, 6, 7, 8, 9]
      r = 'a'..'d'
      r.to_a.should == %w[a b c d]
      r = 'a'...'d'
      r.to_a.should == %w[a b c]
    end

    it 'handles strings' do
      r = 'a'..'z'
      r.to_a.should == %w[a b c d e f g h i j k l m n o p q r s t u v w x y z]
      r = 'z'..'a'
      r.to_a.should == []
      r = 'a'..'a'
      r.to_a.should == ['a']
      r = 'a'...'z'
      r.to_a.should == %w[a b c d e f g h i j k l m n o p q r s t u v w x y]
      r = 'z'...'a'
      r.to_a.should == []
      r = 'a'...'a'
      r.to_a.should == []
    end
  end

  describe '#each' do
    it 'calls the block for each item in the range' do
      items = []
      (1..10).each { |i| items << i }
      items.should == [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
      items = []
      (1...10).each { |i| items << i }
      items.should == [1, 2, 3, 4, 5, 6, 7, 8, 9]
      items = []
      ('a'..'d').each { |i| items << i }
      items.should == %w[a b c d]
    end

    it 'has empty range when first less than last' do
      items = []
      (0...0).each { |i| items << i }
      items.should == []
      items = []
      (0...-10).each { |i| items << i }
      items.should == []
    end

    it 'handles strings' do
      ary = ->(r) do
        a = []
        r.each { |i| a << i }
        a
      end
      r = 'a'..'z'
      ary.(r).should == %w[a b c d e f g h i j k l m n o p q r s t u v w x y z]
      r = 'z'..'a'
      ary.(r).should == []
      r = 'a'..'a'
      ary.(r).should == ['a']
      r = 'a'...'z'
      ary.(r).should == %w[a b c d e f g h i j k l m n o p q r s t u v w x y]
      r = 'z'...'a'
      ary.(r).should == []
      r = 'a'...'a'
      ary.(r).should == []
    end
  end

  describe '#inspect' do
    it 'returns range syntax' do
      (1..10).inspect.should == '1..10'
      (1...10).inspect.should == '1...10'
      ('a'..'z').inspect.should == '"a".."z"'
      ('a'...'z').inspect.should == '"a"..."z"'
    end
  end

  describe '#==' do
    it 'returns true if the ranges are equal' do
      (('a'..'d') == ('a'..'d')).should == true
      ((1..10) == (1..10)).should == true
      ((1...10) == (1...10)).should == true
      x = 10
      y = 20
      ((10..20) == (x..y)).should == true
    end

    it 'returns false if the ranges are not equal' do
      (('a'..'d') == ('a'..'e')).should == false
      (('a'..'d') == ('a'...'d')).should == false
      ((1..10) == (1..9)).should == false
      ((1..10) == (1...10)).should == false
      ((1...10) == (1..10)).should == false
    end
  end

  describe '#===' do
    it 'returns true if the argument is in the range' do
      ((1..10) === 1).should == true
      ((1..10) === 5).should == true
      ((1..10) === 10).should == true
    end

    it 'returns false if the argument is not in the range' do
      ((1..10) === 0).should == false
      ((1..10) === 11).should == false
      ((1...10) === 10).should == false
    end

    it 'returns true if the given string is between the first and last item of the range' do
      r = 'a'..'z'
      (r === 'a').should == true
      (r === 'aa').should == true
    end
  end

  describe '#include?' do
    it 'returns true if the given string is in the range with end' do
      r = 'a'..'z'
      r.include?('a').should == true
      r.include?('z').should == true
      r.include?('aa').should == false
    end

    it 'returns true if the given string is in the range without' do
      r = 'a'...'z'
      r.include?('a').should == true
      r.include?('z').should == false
      r.include?('aa').should == false
    end

    it 'returns true if the given number is in the range with end' do
      r = 1..10
      r.include?(9).should == true
      r.include?(9.9).should == true
      r.include?(10).should == true
      r.include?(10.0).should == true
      r.include?(11).should == false
      r.include?(10.1).should == false
    end

    it 'returns true if the given number is in the range without' do
      r = 1...10
      r.include?(9).should == true
      r.include?(9.9).should == true
      r.include?(10).should == false
      r.include?(10.0).should == false
      r.include?(11).should == false
      r.include?(10.1).should == false
    end
  end
end
