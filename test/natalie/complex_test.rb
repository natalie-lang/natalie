require_relative '../spec_helper'

describe 'complex' do
  it 'can be created with bi syntax' do
    r = 3i
    r.should be_kind_of(Complex)
    r.real.should == 0
    r.imaginary.should == 3
  end

  it 'can be created with a+bi syntax' do
    r = 2+3i
    r.should be_kind_of(Complex)
    r.real.should == 2
    r.imaginary.should == 3
  end

  it 'can be created with Kernel#Complex' do
    r = Complex(0, 3)
    r.should be_kind_of(Complex)
    r.real.should == 0
    r.imaginary.should == 3
  end

  it 'does not have have Comparable mixin more than once' do
    Complex.ancestors.count(Comparable).should == 1
  end

  # NATFIXME: Remove this and sync upstream spec once https://github.com/ruby/spec/pull/1007 is merged
  it 'is frozen' do
    r = Complex(1)
    r.frozen?.should == true
  end
end
