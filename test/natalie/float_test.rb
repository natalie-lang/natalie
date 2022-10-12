require_relative '../spec_helper'

describe 'float' do
  it 'transforms high floats to infinity' do
    1e1020.should == Float::INFINITY
    (-1e1020).should == -Float::INFINITY
  end

  describe '#==' do
    it 'compares with bignums' do
      1124000727777607680000.0.should == 1124000727777607680000
    end
  end

  describe '#**' do
    it 'raises to a power' do
      (2.2 ** 3).should be_close(10.648, TOLERANCE)
    end
  end

  describe '#to_r' do
    it 'returns a rational' do
      2.0.to_r.should == Rational(2, 1)
      2.5.to_r.should == Rational(5, 2)
      -0.75.to_r.should == Rational(-3, 4)
      0.0.to_r.should == Rational(0, 1)
      0.3.to_r.should == Rational(5404319552844595, 18014398509481984)
    end
  end
end
