require_relative '../spec_helper'

describe 'equality' do
  it 'works for every object' do
    o = Object.new
    r = o == o
    r.should == true
    r = o === o
    r.should == true
    r = o.eql?(o)
    r.should == true
    r = o.equal?(o)
    r.should == true
    r = 1 == 1
    r.should == true
    r = 1 === 1
    r.should == true
    r = 'hi' == 'hi'
    r.should == true
    r = 'hi' === 'hi'
    r.should == true
    r = '' == ''
    r.should == true
    r = '' === ''
    r.should == true
    r = :foo == :foo
    r.should == true
    r = :foo === :foo
    r.should == true
    r = true == true
    r.should == true
    r = true === true
    r.should == true
    r = false == false
    r.should == true
    r = false === false
    r.should == true
    r = [1, 2, 3] == [1, 2, 3]
    r.should == true
    r = [1, 2, 3] === [1, 2, 3]
    r.should == true
    r = [] == []
    r.should == true
    r = [] === []
    r.should == true
    r = Object === Object.new
    r.should == true
    r = Object === 1
    r.should == true
    r = Object === 'foo'
    r.should == true
    r = Object === []
    r.should == true
    r = Integer === 1
    r.should == true
    r = String === 'foo'
    r.should == true
    r = Array === []
    r.should == true
    r = 1 != 2
    r.should == true
    o2 = Object.new
    r = o == o2
    r.should == false
    r = o === o2
    r.should == false
    r = o.eql?(o2)
    r.should == false
    r = o.equal?(o2)
    r.should == false
    r = 1 == nil
    r.should == false
    r = 1 === nil
    r.should == false
    r = 1 == '1'
    r.should == false
    r = 1 === '1'
    r.should == false
    r = 'hi' == 'hi there'
    r.should == false
    r = 'hi' === 'hi there'
    r.should == false
    r = 'hi' == nil
    r.should == false
    r = 'hi' === nil
    r.should == false
    r = 'hi' == 0
    r.should == false
    r = 'hi' === 0
    r.should == false
    r = :foo == :bar
    r.should == false
    r = :foo === :bar
    r.should == false
    r = true == false
    r.should == false
    r = true === false
    r.should == false
    r = [1, 2, 3] == [1, 2]
    r.should == false
    r = [1, 2, 3] === [1, 2]
    r.should == false
    r = [1, 2, 3] == [1, 2, 3, 4]
    r.should == false
    r = [1, 2, 3] === [1, 2, 3, 4]
    r.should == false
    r = Integer === []
    r.should == false
    r = String === 1
    r.should == false
    r = Array === 'foo'
    r.should == false
    r = 1 != 1
    r.should == false
  end
end
