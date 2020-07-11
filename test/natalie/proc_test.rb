require_relative '../spec_helper'

describe 'Proc' do
  describe '.new' do
    it 'creates a new Proc object' do
      p = Proc.new do
        'hello from proc'
      end
      p.should be_kind_of(Proc)
    end
  end

  describe 'proc method' do
    it 'creates a new Proc object' do
      p = proc do
        'hello from proc'
      end
      p.should be_kind_of(Proc)
    end
  end

  describe 'lambda method' do
    it 'creates a new Proc object' do
      p = lambda do
        'hello from proc'
      end
      p.should be_kind_of(Proc)
    end
  end

  describe '#call' do
    it 'evaluates the proc and returns the result' do
      p = Proc.new do
        'hello from proc'
      end
      p.call.should == 'hello from proc'
    end
  end

  describe '#lambda?' do
    it 'returns false if the Proc is not a lambda' do
      p = Proc.new do
        'hello from proc'
      end
      p.lambda?.should == false
      p = proc do
        'hello from proc'
      end
      p.lambda?.should == false
    end

    it 'returns true if the Proc is a lambda' do
      p = -> do
        'hello from lambda'
      end
      p.lambda?.should == true
      p = lambda do
        'hello from lambda'
      end
      p.lambda?.should == true
    end
  end
end
