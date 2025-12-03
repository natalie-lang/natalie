require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe 'Comparable#clamp' do
  it 'raises an Argument error unless the 2 parameters are correctly ordered' do
    one = ComparableSpecs::WithOnlyCompareDefined.new(1)
    two = ComparableSpecs::WithOnlyCompareDefined.new(2)
    c = ComparableSpecs::Weird.new(3)

    -> { c.clamp(two, one) }.should raise_error(ArgumentError)
    one.should_receive(:<=>).any_number_of_times.and_return(nil)
    -> { c.clamp(one, two) }.should raise_error(ArgumentError)
  end

  it 'returns self if within the given parameters' do
    one = ComparableSpecs::WithOnlyCompareDefined.new(1)
    two = ComparableSpecs::WithOnlyCompareDefined.new(2)
    three = ComparableSpecs::WithOnlyCompareDefined.new(3)
    c = ComparableSpecs::Weird.new(2)

    c.clamp(one, two).should equal(c)
    c.clamp(two, two).should equal(c)
    c.clamp(one, three).should equal(c)
    c.clamp(two, three).should equal(c)
  end

  it 'returns the min parameter if less than it' do
    one = ComparableSpecs::WithOnlyCompareDefined.new(1)
    two = ComparableSpecs::WithOnlyCompareDefined.new(2)
    c = ComparableSpecs::Weird.new(0)

    c.clamp(one, two).should equal(one)
  end

  it 'returns the max parameter if greater than it' do
    one = ComparableSpecs::WithOnlyCompareDefined.new(1)
    two = ComparableSpecs::WithOnlyCompareDefined.new(2)
    c = ComparableSpecs::Weird.new(3)

    c.clamp(one, two).should equal(two)
  end

  it 'returns self if within the given range parameters' do
    one = ComparableSpecs::WithOnlyCompareDefined.new(1)
    two = ComparableSpecs::WithOnlyCompareDefined.new(2)
    three = ComparableSpecs::WithOnlyCompareDefined.new(3)
    c = ComparableSpecs::Weird.new(2)

    c.clamp(one..two).should equal(c)
    c.clamp(two..two).should equal(c)
    c.clamp(one..three).should equal(c)
    c.clamp(two..three).should equal(c)
  end

  it 'returns the minimum value of the range parameters if less than it' do
    one = ComparableSpecs::WithOnlyCompareDefined.new(1)
    two = ComparableSpecs::WithOnlyCompareDefined.new(2)
    c = ComparableSpecs::Weird.new(0)

    c.clamp(one..two).should equal(one)
  end

  it 'returns the maximum value of the range parameters if greater than it' do
    one = ComparableSpecs::WithOnlyCompareDefined.new(1)
    two = ComparableSpecs::WithOnlyCompareDefined.new(2)
    c = ComparableSpecs::Weird.new(3)

    c.clamp(one..two).should equal(two)
  end

  it 'raises an Argument error if the range parameter is exclusive' do
    one = ComparableSpecs::WithOnlyCompareDefined.new(1)
    two = ComparableSpecs::WithOnlyCompareDefined.new(2)
    c = ComparableSpecs::Weird.new(3)

    -> { c.clamp(one...two) }.should raise_error(ArgumentError)
  end

  context 'with nil as the max argument' do
    it 'returns min argument if less than it' do
      one = ComparableSpecs::WithOnlyCompareDefined.new(1)
      zero = ComparableSpecs::WithOnlyCompareDefined.new(0)
      c = ComparableSpecs::Weird.new(0)

      c.clamp(one, nil).should equal(one)
      c.clamp(zero, nil).should equal(c)
    end

    it 'always returns self if greater than min argument' do
      one = ComparableSpecs::WithOnlyCompareDefined.new(1)
      two = ComparableSpecs::WithOnlyCompareDefined.new(2)
      c = ComparableSpecs::Weird.new(2)

      c.clamp(one, nil).should equal(c)
      c.clamp(two, nil).should equal(c)
    end
  end

  context 'with endless range' do
    it 'returns minimum value of the range parameters if less than it' do
      one = ComparableSpecs::WithOnlyCompareDefined.new(1)
      zero = ComparableSpecs::WithOnlyCompareDefined.new(0)
      c = ComparableSpecs::Weird.new(0)

      NATFIXME 'it returns minimum value of the range parameters if less than it', exception: NoMethodError, message: "undefined method 'value' for nil" do
        c.clamp(one..).should equal(one)
        c.clamp(zero..).should equal(c)
      end
    end

    it 'always returns self if greater than minimum value of the range parameters' do
      one = ComparableSpecs::WithOnlyCompareDefined.new(1)
      two = ComparableSpecs::WithOnlyCompareDefined.new(2)
      c = ComparableSpecs::Weird.new(2)

      NATFIXME 'it always returns self if greater than minimum value of the range parameters', exception: NoMethodError, message: "undefined method 'value' for nil" do
        c.clamp(one..).should equal(c)
        c.clamp(two..).should equal(c)
      end
    end

    it 'works with exclusive range' do
      one = ComparableSpecs::WithOnlyCompareDefined.new(1)
      c = ComparableSpecs::Weird.new(2)

      NATFIXME 'it works with exclusive range', exception: ArgumentError, message: 'cannot clamp with an exclusive range' do
        c.clamp(one...).should equal(c)
      end
    end
  end

  context 'with nil as the min argument' do
    it 'returns max argument if greater than it' do
      one = ComparableSpecs::WithOnlyCompareDefined.new(1)
      c = ComparableSpecs::Weird.new(2)

      c.clamp(nil, one).should equal(one)
    end

    it 'always returns self if less than max argument' do
      one = ComparableSpecs::WithOnlyCompareDefined.new(1)
      zero = ComparableSpecs::WithOnlyCompareDefined.new(0)
      c = ComparableSpecs::Weird.new(0)

      c.clamp(nil, one).should equal(c)
      c.clamp(nil, zero).should equal(c)
    end
  end

  context 'with beginless range' do
    it 'returns maximum value of the range parameters if greater than it' do
      one = ComparableSpecs::WithOnlyCompareDefined.new(1)
      c = ComparableSpecs::Weird.new(2)

      NATFIXME 'it returns maximum value of the range parameters if greater than it', exception: ArgumentError, message: 'min argument must be smaller than max argument' do
        c.clamp(..one).should equal(one)
      end
    end

    it 'always returns self if less than maximum value of the range parameters' do
      one = ComparableSpecs::WithOnlyCompareDefined.new(1)
      zero = ComparableSpecs::WithOnlyCompareDefined.new(0)
      c = ComparableSpecs::Weird.new(0)

      NATFIXME 'it always returns self if less than maximum value of the range parameters', exception: ArgumentError, message: 'min argument must be smaller than max argument' do
        c.clamp(..one).should equal(c)
        c.clamp(..zero).should equal(c)
      end
    end

    it 'raises an Argument error if the range parameter is exclusive' do
      one = ComparableSpecs::WithOnlyCompareDefined.new(1)
      c = ComparableSpecs::Weird.new(0)

      -> { c.clamp(...one) }.should raise_error(ArgumentError)
    end
  end

  context 'with nil as the min and the max argument' do
    it 'always returns self' do
      c = ComparableSpecs::Weird.new(1)

      c.clamp(nil, nil).should equal(c)
    end
  end

  context 'with beginless-and-endless range' do
    it 'always returns self' do
      c = ComparableSpecs::Weird.new(1)

      NATFIXME 'it always returns self', exception: NoMethodError, message: "undefined method 'value' for nil" do
        c.clamp(nil..nil).should equal(c)
      end
    end

    it 'works with exclusive range' do
      c = ComparableSpecs::Weird.new(2)

      NATFIXME 'it works with exclusive range', exception: ArgumentError, message: 'cannot clamp with an exclusive range' do
        c.clamp(nil...nil).should equal(c)
      end
    end
  end
end
