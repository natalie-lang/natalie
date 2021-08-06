require_relative '../spec_helper'

describe 'booleans' do
  it 'cannot be created with .new' do
    -> { TrueClass.new }.should raise_error(NoMethodError)
    -> { FalseClass.new }.should raise_error(NoMethodError)
  end

  describe 'equality' do
    it 'is equal to other booleans' do
      r = true == true
      r.should == true
      r = false == false
      r.should == true
      r = true == false
      r.should == false
      r = false == true
      r.should == false
    end
  end

  describe 'inspect' do
    it 'returns its name' do
      true.inspect.should == 'true'
      false.inspect.should == 'false'
    end
  end

  describe 'and' do
    it 'only evaluates left side once' do
      @x = 0
      x = -> { @x += 1; false }
      r = x.() and 1
      r.should == false
      @x.should == 1
    end
  end

  describe '&&' do
    it 'only evaluates left side once' do
      @x = 0
      x = -> { @x += 1; false }
      r = x.() && 1
      r.should == false
      @x.should == 1
    end
  end

  describe 'or' do
    it 'only evaluates left side once' do
      @x = 0
      x = -> { @x += 1; true }
      r = x.() or 1
      r.should == true
      @x.should == 1
    end
  end

  describe '||' do
    it 'only evaluates left side once' do
      @x = 0
      x = -> { @x += 1; true }
      r = x.() || 1
      r.should == true
      @x.should == 1
    end
  end
end
