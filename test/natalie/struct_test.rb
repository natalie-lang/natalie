require_relative '../spec_helper'

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

  it 'can be iterated' do
    s = Struct.new(:a, :b, :c)
    i = s.new(1, 2, 3)
    collected = []
    i.each { |val| collected << val }
    collected.should == [1, 2, 3]
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
end
