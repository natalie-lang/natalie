require_relative '../spec_helper'

describe 'case/in' do
  it 'sets variables' do
    result = case 1
             in x
               x * 2
             in y
               :bad
             end
    result.should == 2
  end

  it 'matches literals' do
    match = ->(value) {
      case value
      in 1
        :int
      in 1.1
        :float
      in 'str'
        :string
      in :sym
        :symbol
      end
    }
    match.(1).should == :int
    match.(1.1).should == :float
    match.('str').should == :string
    match.(:sym).should == :symbol
    match.(123456).should == nil
  end

  #it 'works with arrays' do
    ##result = case []
             ##in []
               ##:empty
             ##end
    ##result.should == :empty
    #result = case [2]
             #in []
               #:bad
             #in [3]
               #:bad
             #in [x]
               #x * 3
             #else
               #:bad
             #end
    #result.should == 6
  #end
end
