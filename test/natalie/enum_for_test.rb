require_relative '../spec_helper'

class Collection
  include Enumerable

  attr_reader :arity

  def initialize(*arr)
    @arr = arr
  end

  def each(&block)
    return enum_for(:each) unless block_given?
    @arr.each(&block)
  end

  def mockingbird_each(&block)
    return enum_for(:mockingbird_each) unless block_given?
    @arity = block.arity
    each { |x| block[x, x] }
  end
end

describe 'enum_for arity validation' do
  it 'returns the correct arity for the block' do
    collection = Collection.new
    collection.mockingbird_each { |x, y| x + y }
    collection.arity.should == 2

    collection.mockingbird_each.each { |x, y| x + y }
    if RUBY_ENGINE == 'natalie'
      # NATFIXME: No the desired behaviour, but we'll keep it during the rewrite to C++ to validate
      #           that we made no behavioural changes.
      collection.arity.should == -1
    else
      collection.arity.should == 2
    end
  end
end
