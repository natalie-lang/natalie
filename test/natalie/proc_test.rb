require_relative '../spec_helper'

describe 'Proc' do
  describe '.new' do
    it 'creates a new Proc object' do
      p = Proc.new { 'hello from proc' }
      p.should be_kind_of(Proc)
    end
  end

  describe 'proc method' do
    it 'creates a new Proc object' do
      p = proc { 'hello from proc' }
      p.should be_kind_of(Proc)
    end
  end

  describe 'lambda method' do
    it 'creates a new Proc object' do
      p = lambda { 'hello from proc' }
      p.should be_kind_of(Proc)
    end
  end

  describe '-> operator' do
    it 'creates a Proc object' do
      p = -> { 'hello from proc' }
      p.should be_kind_of(Proc)
    end
  end

  describe '#call' do
    it 'evaluates the proc and returns the result' do
      p = Proc.new { 'hello from proc' }
      p.call.should == 'hello from proc'
    end
  end

  describe '#lambda?' do
    it 'returns false if the Proc is not a lambda' do
      p = Proc.new { 'hello from proc' }
      p.lambda?.should == false
      p = proc { 'hello from proc' }
      p.lambda?.should == false
    end

    it 'returns true if the Proc is a lambda' do
      p = -> { 'hello from lambda' }
      p.lambda?.should == true
      p = lambda { 'hello from lambda' }
      p.lambda?.should == true
    end
  end

  describe '#arity' do
    it 'returns the correct number of required arguments' do
      Proc.new {}.arity.should == 0
      Proc.new { |x| }.arity.should == 1
      Proc.new { |x, y = 1| }.arity.should == 1
      Proc.new { |x, y = 1, a:| }.arity.should == 2
      Proc.new { |x, y = 1, a: nil, b:| }.arity.should == 2
      Proc.new { |x, y = 1, a: nil, b: nil| }.arity.should == 1
    end
  end

  describe '#to_proc' do
    it 'returns self' do
      p = Proc.new {}
      p.to_proc.object_id.should == p.object_id
      p.to_proc.lambda?.should == false
      l = -> {  }
      l.to_proc.object_id.should == l.object_id # does not convert
      l.to_proc.lambda?.should == true # does not change to false
    end
  end
end
