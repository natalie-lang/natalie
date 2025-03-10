require_relative '../../spec_helper'

describe 'Range#minmax' do
  before(:each) do
    @x = mock('x')
    @y = mock('y')

    @x.should_receive(:<=>).with(@y).any_number_of_times.and_return(-1) # x < y
    @x.should_receive(:<=>).with(@x).any_number_of_times.and_return(0) # x == x
    @y.should_receive(:<=>).with(@x).any_number_of_times.and_return(1) # y > x
    @y.should_receive(:<=>).with(@y).any_number_of_times.and_return(0) # y == y
  end

  describe 'on an inclusive range' do
    it 'should raise RangeError on an endless range without iterating the range' do
      @x.should_not_receive(:succ)

      range = (@x..)

      -> { range.minmax }.should raise_error(RangeError, 'cannot get the maximum of endless range')
    end

    it 'raises RangeError or ArgumentError on a beginless range' do
      range = (..@x)

      -> { range.minmax }.should raise_error(StandardError) { |e|
        if RangeError === e
          # error from #min
          -> { raise e }.should raise_error(RangeError, 'cannot get the minimum of beginless range')
        else
          # error from #max
          -> { raise e }.should raise_error(ArgumentError, 'comparison of NilClass with MockObject failed')
        end
      }
    end

    it 'should return beginning of range if beginning and end are equal without iterating the range' do
      @x.should_not_receive(:succ)

      (@x..@x).minmax.should == [@x, @x]
    end

    it 'should return nil pair if beginning is greater than end without iterating the range' do
      @y.should_not_receive(:succ)

      (@y..@x).minmax.should == [nil, nil]
    end

    it 'should return the minimum and maximum values for a non-numeric range without iterating the range' do
      @x.should_not_receive(:succ)

      (@x..@y).minmax.should == [@x, @y]
    end

    it 'should return the minimum and maximum values for a numeric range' do
      (1..3).minmax.should == [1, 3]
    end

    it 'should return the minimum and maximum values for a numeric range without iterating the range' do
      # We cannot set expectations on integers,
      # so we "prevent" iteration by picking a value that would iterate until the spec times out.
      range_end = Float::INFINITY

      (1..range_end).minmax.should == [1, range_end]
    end

    it 'should return the minimum and maximum values according to the provided block by iterating the range' do
      @x.should_receive(:succ).once.and_return(@y)

      (@x..@y).minmax { |x, y| - (x <=> y) }.should == [@y, @x]
    end
  end

  describe 'on an exclusive range' do
    it 'should raise RangeError on an endless range' do
      @x.should_not_receive(:succ)
      range = (@x...)

      -> { range.minmax }.should raise_error(RangeError, 'cannot get the maximum of endless range')
    end

    it 'should raise RangeError on a beginless range' do
      range = (...@x)

      -> { range.minmax }.should raise_error(RangeError,
        /cannot get the maximum of beginless range with custom comparison method|cannot get the minimum of beginless range/)
    end

    ruby_bug "#17014", ""..."3.0" do
      it 'should return nil pair if beginning and end are equal without iterating the range' do
        NATFIXME 'it should return nil pair if beginning and end are equal without iterating the range', exception: NoMethodError, message: /undefined method [`']<' for an instance of MockObject/ do
          @x.should_not_receive(:succ)

          (@x...@x).minmax.should == [nil, nil]
        end
      end

      it 'should return nil pair if beginning is greater than end without iterating the range' do
        NATFIXME 'it should return nil pair if beginning is greater than end without iterating the range', exception: NoMethodError, message: /undefined method [`']<' for an instance of MockObject/ do
          @y.should_not_receive(:succ)

          (@y...@x).minmax.should == [nil, nil]
        end
      end

      it 'should return the minimum and maximum values for a non-numeric range by iterating the range' do
        NATFIXME 'it should return the minimum and maximum values for a non-numeric range by iterating the range', exception: NoMethodError, message: /undefined method [`']<' for an instance of MockObject/ do
          @x.should_receive(:succ).once.and_return(@y)

          (@x...@y).minmax.should == [@x, @x]
        end
      end
    end

    it 'should return the minimum and maximum values for a numeric range' do
      (1...3).minmax.should == [1, 2]
    end

    it 'should return the minimum and maximum values for a numeric range without iterating the range' do
      # We cannot set expectations on integers,
      # so we "prevent" iteration by picking a value that would iterate until the spec times out.
      range_end = bignum_value

      (1...range_end).minmax.should == [1, range_end - 1]
    end

    it 'raises TypeError if the end value is not an integer' do
      range = (0...Float::INFINITY)
      -> { range.minmax }.should raise_error(TypeError, 'cannot exclude non Integer end value')
    end

    it 'should return the minimum and maximum values according to the provided block by iterating the range' do
      @x.should_receive(:succ).once.and_return(@y)

      (@x...@y).minmax { |x, y| - (x <=> y) }.should == [@x, @x]
    end
  end
end
