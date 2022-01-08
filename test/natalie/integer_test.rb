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

    it 'handles bignums correctly' do
      bignum = bignum_value
      (bignum % -50).should == -42
      (bignum % -500).should == -192
      (bignum % -999).should == -919
      (bignum % -1009).should == -817
      (-bignum % 999).should == 919
      (-bignum % -999).should == -80
    end
  end

  describe '** (exponentiation)' do
    it 'raises the number to the given power' do
      (2**2).should == 4
      (2**3).should == 8
      (2**4).should == 16
    end
  end

  describe '*' do
    it 'works for integers' do
      (2 * 3).should == 6
    end

    it 'works for floats' do
      (2 * 2.5).should == 5.0
    end

    it 'detect overflows correctly' do
      (fixnum_max * fixnum_max).should > fixnum_max
      (fixnum_max * -fixnum_max).should < -fixnum_max
      (-fixnum_max * fixnum_max).should < -fixnum_max
      (-fixnum_max * -fixnum_max).should > fixnum_max
    end
  end

  describe '+' do
    it 'works for integers' do
      (2 + 3).should == 5
    end

    it 'works for floats' do
      (2 + 2.5).should == 4.5
    end

    it 'detect overflows correctly' do
      (fixnum_max + 2).should > fixnum_max
      (2 + fixnum_max).should > fixnum_max
      (-fixnum_max + -2).should < -fixnum_max
      (-2 + -fixnum_max).should < -fixnum_max
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

    it 'detect overflows correctly' do
      (fixnum_max - -2).should > fixnum_max
      (2 - -fixnum_max).should > fixnum_max
      (-2 - fixnum_max).should < -fixnum_max
      (-fixnum_max - 2).should < -fixnum_max
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

  describe '==' do
    (2 == 2).should == true
    (2 == 3).should == false
  end

  describe 'eql?' do
    (2.eql?(2)).should == true
    (2.eql?(3)).should == false
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

  describe '#odd?' do
    specify do
      -2.odd?.should == false
      -1.odd?.should == true
      0.odd?.should == false
      1.odd?.should == true
      2.odd?.should == false
    end
  end

  describe '#even?' do
    specify do
      -2.even?.should == true
      -1.even?.should == false
      0.even?.should == true
      1.even?.should == false
      2.even?.should == true
    end
  end

  describe '#zero?' do
    specify do
      0.zero?.should == true
      1.zero?.should == false
      -1.zero?.should == false
    end
  end

  describe '#times' do
    it 'handles bignums correctly' do
      bignum_value.times.size.should == bignum_value

      # We just test if the loop "starts" (iterating until we reach bignum sizes would take ages)
      i = 0
      bignum_value.times do |n|
        i += 1
        break if i == 5
      end
      i.should == 5

      (-bignum_value).times.size.should == 0
      i = (-bignum_value).times {}
      i.should == -bignum_value
    end
  end

  describe '#to_s' do
    it 'returns a base-10 string representation' do
      -10.to_s.should == '-10'
      0.to_s.should == '0'
      1981.to_s.should == '1981'
    end

    it 'given a base, returns a string representation of that base' do
      0.to_s(16).should == '0'
      1981.to_s(16).should == '7bd'
      -1981.to_s(16).should == '-7bd'
    end
  end
end
