require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe 'Enumerable#sum' do
  before :each do
    @enum = Object.new.to_enum
    class << @enum
      def each
        yield 0
        yield(-1)
        yield 2
        # NATFIXME: Re-add rationals if implemented
        # yield 2/3r
      end
    end
  end

  it 'returns amount of the elements with taking an argument as the initial value' do
    # @enum.sum(10).should == 35/3r
    @enum.sum(10).should == 11
  end

  it 'gives 0 as a default argument' do
    # @enum.sum.should == 5/3r
    @enum.sum.should == 1
  end

  it 'takes a block to transform the elements' do
    # @enum.sum { |element| element * 2 }.should == 10/3r
    @enum.sum { |element| element * 2 }.should == 2
  end
end
