require_relative '../spec_helper'

class RespondToTest
end

describe 'Kernel#respond_to?' do
  %i[>= > <= <].each do |method|
    it "is used by Hash##{method}" do
      hash = { baz: 'qux' }

      [true, 1].each do |truthy_value|
        o1 = Object.new
        o1.should_receive(:respond_to?).with(:to_hash, true).and_return(truthy_value)
        o1.should_receive(:to_hash).and_return({ foo: 'bar' })
        -> { hash.send(method, o1) }.should_not raise_error
      end

      [false, nil].each do |falsey_value|
        o2 = Object.new
        o2.should_receive(:respond_to?).with(:to_hash, true).and_return(falsey_value)
        o2.should_not_receive(:to_hash)
        -> { hash.send(method, o2) }.should raise_error(TypeError, /no implicit conversion of Object into Hash/)
      end
    end
  end

  it 'is used by BasicObject#allocate' do
    [true, 1].each do |truthy_value|
      RespondToTest.should_receive(:respond_to?).with(:allocate, true).and_return(truthy_value)
      -> { RespondToTest.allocate }.should_not raise_error
    end

    [false, nil].each do |falsey_value|
      RespondToTest.should_receive(:respond_to?).with(:allocate, true).and_return(falsey_value)
      -> { RespondToTest.allocate }.should raise_error(TypeError, /calling RespondToTest.allocate is prohibited/)
    end
  end

  it 'is used by Array#initialize' do
    [true, 1].each do |truthy_value|
      o1 = Object.new
      o1.should_receive(:respond_to?).with(:to_ary, true).and_return(truthy_value)
      o1.should_receive(:to_ary).and_return(%w[foo bar])
      -> { Array.new(o1) }.should_not raise_error
    end

    [false, nil].each do |falsey_value|
      o2 = Object.new
      o2.should_receive(:respond_to?).with(:to_ary, true).and_return(falsey_value)
      o2.should_not_receive(:to_ary)
      -> { Array.new(o2) }.should raise_error(TypeError, /no implicit conversion of Object into Integer/)
    end
  end

  it 'is used by Kernel#Array' do
    [true, 1].each do |truthy_value|
      o1 = Object.new
      o1.should_receive(:respond_to?).with(:to_ary, true).and_return(truthy_value)
      o1.should_receive(:to_ary).and_return(%w[foo bar])
      Array(o1).should == %w[foo bar]
    end

    [false, nil].each do |falsey_value|
      o2 = Object.new
      o2.should_receive(:respond_to?).with(:to_ary, true).and_return(falsey_value)
      o2.should_not_receive(:to_ary)
      Array(o2).should == [o2]
    end
  end
end
