require_relative '../spec_helper'

describe 'rational' do
  it 'transforms rational lit to rational object' do
    1r.should == Rational(1)
    -1r.should == Rational(-1)
    (2/3r).should == Rational(2, 3)
    (-2/3r).should == Rational(-2, 3)
    (2/-3r).should == Rational(2, -3)

    reduced = 20/6r
    reduced.numerator.should == 10
    reduced.denominator.should == 3
  end

  it 'is frozen' do
    r = Rational(1)
    r.frozen?.should == true
  end
end
