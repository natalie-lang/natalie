require_relative '../spec_helper'

class Foo < Struct; end
class Bar < Foo; end

describe 'Struct' do
  it 'can be created' do
    s = Struct.new(:a, :b)
    s.superclass.should == Struct
  end

  it 'has an initializer that accepts arguments' do
    s = Struct.new(:a, :b)
    s.new(1, 2)
  end

  context 'keyword_init' do
    it 'can be created' do
      s = Struct.new(:a, :b, keyword_init: true)
      s.superclass.should == Struct
    end

    it 'has an initializer that accepts argument keywords' do
      s = Struct.new(:a, :b, keyword_init: true)
      i = s.new(b: 1, a: 2)
      i.a.should == 2
      i.b.should == 1
    end
  end

  it 'has accessors' do
    s = Struct.new(:a, :b)
    i = s.new(1, 2)
    i.a.should == 1
    i.b.should == 2
    i.a = 100
    i.a.should == 100
  end

  describe '#each' do
    it 'iterates the struct when a block is given' do
      s = Struct.new(:a, :b, :c)
      i = s.new(1, 2, 3)
      collected = []
      i.each { |val| collected << val }
      collected.should == [1, 2, 3]
    end

    it 'returns an enumerator when no block is given' do
      s = Struct.new(:a, :b, :c)
      i = s.new(1, 2, 3)
      iter = i.each
      iter.should be_an_instance_of(Enumerator)
      iter.to_a.should == [1, 2, 3]
    end
  end

  describe '#each_pair' do
    it 'iterates the attribute/value pairs' do
      s = Struct.new(:a, :b, :c)
      i = s.new(1, 2, 3)
      collected = []
      i.each_pair { |key, val| collected << [key, val] }
      collected.should == [[:a, 1], [:b, 2], [:c, 3]]
    end

    it 'returns an enumerator when no block is given' do
      s = Struct.new(:a, :b, :c)
      i = s.new(1, 2, 3)
      iter = i.each_pair
      iter.should be_an_instance_of(Enumerator)
      iter.to_a.should == [[:a, 1], [:b, 2], [:c, 3]]
    end
  end

  it 'is an Enumerable' do
    s = Struct.new(:a, :b, :c, :d)
    i = s.new(1, 2, 3, 4)
    i.select { |v| v <= 2 }.should == [1, 2]
  end

  it 'can be inspected' do
    s = Struct.new(:a, :b, :c, :d)
    i = s.new(1, 2.3, 'foo', nil)
    i.inspect.should == "#<struct a=1, b=2.3, c=\"foo\", d=nil>"
  end

  it 'can subclass Struct' do
    c = Class.new(Struct)
    s = c.new(:a, :b)
    i = s.new(1, 2)
    i.a.should == 1
    i.b.should == 2

    s = Bar.new(:a, :b)
    i = s.new(1, 2)
    i.a.should == 1
    i.b.should == 2
  end

  it 'does a sanity check on the argument of #[]' do
    s = Struct.new(:a, :b)
    i = s.new(1)
    -> { i[:object_id] }.should raise_error(NameError, /no member 'object_id' in struct/)
    -> { i[2] }.should raise_error(IndexError)
  end
end
