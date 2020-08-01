require_relative '../spec_helper'

describe 'integer' do
  describe '#succ' do
    it 'returns the next successive integer' do
      -1.succ.should == 0
      1.succ.should == 2
      100.succ.should == 101
    end
  end

  describe 'modulo' do
    it 'returns the modulo result' do
      (6 % 2).should == 0
      (6 % 3).should == 0
      (6 % 4).should == 2
      (6 % 5).should == 1
    end
  end

  describe '** (exponentiation)' do
    it 'raises the number to the given power' do
      (2 ** 2).should == 4
      (2 ** 3).should == 8
      (2 ** 4).should == 16
    end
  end

  describe '*' do
    it 'works for integers' do
      (2 * 3).should == 6
    end

    it 'works for floats' do
      (2 * 2.5).should == 5.0
    end
  end

  describe '+' do
    it 'works for integers' do
      (2 + 3).should == 5
    end

    it 'works for floats' do
      (2 + 2.5).should == 4.5
    end
  end

  describe '-' do
    it 'works for integers' do
      (3 - 2).should == 1
      (2 - 3).should == -1
    end

    it 'works for floats' do
      (2 - 1.0).should == 1.0
      (2 - 2.5).should == -0.5
    end
  end

  describe '/' do
    it 'works for whole quotients' do
      (6 / 2).should == 3
    end

    it 'works for floats' do
      (7 / 2.0).should == 3.5
    end

    it 'returns quotient of integer division rounded down' do
      (3 / 2).should == 1
      (11 / 6).should == 1
    end

    it 'raises ZeroDivisionError when dividing by zero' do
      -> { 2 / 0 }.should raise_error(ZeroDivisionError)
    end
  end

  describe '<' do
    it 'returns true if we are less than the given integer' do
      (1 < 10).should == true
    end

    it 'returns false if we are more than or equal to the given integer' do
      (1 < 1).should == false
      (11 < 10).should == false
      (2 < 1).should == false
    end
  end

  describe '<=' do
    it 'returns true if we are less than or equal to the given integer' do
      (1 <= 10).should == true
      (1 <= 1).should == true
    end

    it 'returns false if we are more than the given integer' do
      (11 <= 10).should == false
      (2 <= 1).should == false
    end
  end

  describe '>' do
    it 'returns true if we are greater than the given integer' do
      (10 > 1).should == true
    end

    it 'returns false if we are less than or equal to the given integer' do
      (1 > 1).should == false
      (10 > 11).should == false
      (1 > 2).should == false
    end
  end

  describe '>=' do
    it 'returns true if we are greater than or equal to the given integer' do
      (10 >= 1).should == true
      (1 >= 1).should == true
    end

    it 'returns false if we are less than the given integer' do
      (10 >= 11).should == false
      (1 >= 2).should == false
    end
  end
end
