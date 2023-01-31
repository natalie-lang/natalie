require_relative '../spec_helper'

describe 'Equals methods on numeric classes' do
  before do
    @no_equals_method = Object.new.tap { |obj| obj.singleton_class.undef_method(:==) }
  end

  it 'should work on Integer' do
    (3 == 3).should be_true
    (3 == 3.0).should be_true
    (3 == Rational(3)).should be_true
    (3 == Complex(3)).should be_true
    (3 == Complex(3.0)).should be_true
    -> { 3 == @no_equals_method }.should raise_error(NoMethodError)
  end

  it 'should work on Float' do
    (3.0 == 3).should be_true
    (3.0 == 3.0).should be_true
    (3.0 == Rational(3)).should be_true
    (3.0 == Complex(3)).should be_true
    (3.0 == Complex(3.0)).should be_true
    # NATFIXME: Should raise NoMethodError
    (3.0 == @no_equals_method).should be_false

    # NATFIXME: These should all be_true
    (0.75 == Rational(3, 4)).should be_false
    (2.0 == Rational(4, 2)).should be_true
    (-2.0 == Rational(-4, 2)).should be_true
    (-2.0 == Rational(4, -2)).should be_true
  end

  it 'should work on Rational' do
    (Rational(3) == 3).should be_true
    (Rational(3) == 3.0).should be_true
    (Rational(3) == Rational(3)).should be_true
    (Rational(3) == Complex(3)).should be_true
    (Rational(3) == Complex(3.0)).should be_true
    -> { Rational(3) == @no_equals_method }.should raise_error(NoMethodError)

    # NATFIXME: These should all be_true
    (Rational(3, 4) == 0.75).should be_false
    (Rational(4, 2) == 2.0).should be_true
    (Rational(-4, 2) == -2.0).should be_true
    (Rational(4, -2) == -2.0).should be_true
    # NATFIXME: Should raise NoMethodError
    (Rational(3, 4) == @no_equals_method).should be_false

    # NATFIXME: This should be be_false
    obj = mock("Object")
    obj.should_receive(:==).and_return(:result)
    (Rational(3, 4) == obj).should_not be_true
    obj == 1 # NATFXIME: Handle expectation to make the test pass
  end

  it 'should work on Complex' do
    (Complex(3) == 3).should be_true
    (Complex(3) == 3.0).should be_true
    (Complex(3) == Rational(3)).should be_true
    (Complex(3) == Complex(3)).should be_true
    (Complex(3) == Complex(3.0)).should be_true
    -> { Complex(3) == @no_equals_method }.should raise_error(NoMethodError)

    (Complex(3.0) == 3).should be_true
    (Complex(3.0) == 3.0).should be_true
    (Complex(3.0) == Rational(3)).should be_true
    (Complex(3.0) == Complex(3)).should be_true
    (Complex(3.0) == Complex(3.0)).should be_true
    # NATFIXME: Should raise NoMethodError
    (Complex(3.0) == @no_equals_method).should be_false
  end
end
