require_relative '../spec_helper'

describe 'Enumerable' do
  describe '#map' do
    it 'returns a new array, yielding each item to the block' do
      class Things
        include Enumerable

        def each
          yield 1
          yield 2
          yield 3
        end
      end

      Things.new.map { |i| i * 2 }.should == [2, 4, 6]
    end
  end
end
