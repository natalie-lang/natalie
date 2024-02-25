# Specs shared by Kernel#Complex() and String#to_c()
describe :kernel_complex, shared: true do

  it "returns a Complex object" do
    @object.send(@method, '9').should be_an_instance_of(Complex)
  end

  it "understands integers" do
    NATFIXME 'understands integers', exception: SpecFailedException do
      @object.send(@method, '20').should == Complex(20)
    end
  end

  it "understands negative integers" do
    NATFIXME 'understands negative integers', exception: SpecFailedException do
      @object.send(@method, '-3').should == Complex(-3)
    end
  end

  it "understands fractions (numerator/denominator) for the real part" do
    NATFIXME 'understands fractions (numerator/denominator) for the real part', exception: SpecFailedException do
      @object.send(@method, '2/3').should == Complex(Rational(2, 3))
    end
  end

  it "understands fractions (numerator/denominator) for the imaginary part" do
    NATFIXME 'understands fractions (numerator/denominator) for the imaginary part', exception: SpecFailedException do
      @object.send(@method, '4+2/3i').should == Complex(4, Rational(2, 3))
    end
  end

  it "understands negative fractions (-numerator/denominator) for the real part" do
    NATFIXME 'understands negative fractions (-numerator/denominator) for the real part', exception: SpecFailedException do
      @object.send(@method, '-2/3').should == Complex(Rational(-2, 3))
    end
  end

  it "understands negative fractions (-numerator/denominator) for the imaginary part" do
    NATFIXME 'understands negative fractions (-numerator/denominator) for the imaginary part', exception: SpecFailedException do
      @object.send(@method, '7-2/3i').should == Complex(7, Rational(-2, 3))
    end
  end

  it "understands floats (a.b) for the real part" do
    NATFIXME 'understands floats (a.b) for the real part', exception: SpecFailedException do
      @object.send(@method, '2.3').should == Complex(2.3)
    end
  end

  it "understands floats (a.b) for the imaginary part" do
    NATFIXME 'understands floats (a.b) for the imaginary part', exception: SpecFailedException do
      @object.send(@method, '4+2.3i').should == Complex(4, 2.3)
    end
  end

  it "understands negative floats (-a.b) for the real part" do
    NATFIXME 'understands negative floats (-a.b) for the real part', exception: SpecFailedException do
      @object.send(@method, '-2.33').should == Complex(-2.33)
    end
  end

  it "understands negative floats (-a.b) for the imaginary part" do
    NATFIXME 'understands negative floats (-a.b) for the imaginary part', exception: SpecFailedException do
      @object.send(@method, '7-28.771i').should == Complex(7, -28.771)
    end
  end

  it "understands an integer followed by 'i' to mean that integer is the imaginary part" do
    NATFIXME "understands an integer followed by 'i' to mean that integer is the imaginary part", exception: SpecFailedException do
      @object.send(@method, '35i').should == Complex(0,35)
    end
  end

  it "understands a negative integer followed by 'i' to mean that negative integer is the imaginary part" do
    NATFIXME "understands a negative integer followed by 'i' to mean that negative integer is the imaginary part", exception: SpecFailedException do
      @object.send(@method, '-29i').should == Complex(0,-29)
    end
  end

  it "understands an 'i' by itself as denoting a complex number with an imaginary part of 1" do
    NATFIXME "understands an 'i' by itself as denoting a complex number with an imaginary part of 1", exception: SpecFailedException do
      @object.send(@method, 'i').should == Complex(0,1)
    end
  end

  it "understands a '-i' by itself as denoting a complex number with an imaginary part of -1" do
    NATFIXME "understands a '-i' by itself as denoting a complex number with an imaginary part of -1", exception: SpecFailedException do
      @object.send(@method, '-i').should == Complex(0,-1)
    end
  end

  it "understands 'a+bi' to mean a complex number with 'a' as the real part, 'b' as the imaginary" do
    NATFIXME "understands 'a+bi' to mean a complex number with 'a' as the real part, 'b' as the imaginary", exception: SpecFailedException do
      @object.send(@method, '79+4i').should == Complex(79,4)
    end
  end

  it "understands 'a-bi' to mean a complex number with 'a' as the real part, '-b' as the imaginary" do
    NATFIXME "understands 'a-bi' to mean a complex number with 'a' as the real part, '-b' as the imaginary", exception: SpecFailedException do
      @object.send(@method, '79-4i').should == Complex(79,-4)
    end
  end

  it "understands 'a+i' to mean a complex number with 'a' as the real part, 1i as the imaginary" do
    NATFIXME "understands 'a+i' to mean a complex number with 'a' as the real part, 1i as the imaginary", exception: SpecFailedException do
      @object.send(@method, '79+i').should == Complex(79, 1)
    end
  end

  it "understands 'a-i' to mean a complex number with 'a' as the real part, -1i as the imaginary" do
    NATFIXME "understands 'a-i' to mean a complex number with 'a' as the real part, -1i as the imaginary", exception: SpecFailedException do
      @object.send(@method, '79-i').should == Complex(79, -1)
    end
  end

  it "understands i, I, j, and J imaginary units" do
    NATFIXME 'understands i, I, j, and J imaginary units', exception: SpecFailedException do
      @object.send(@method, '79+4i').should == Complex(79, 4)
      @object.send(@method, '79+4I').should == Complex(79, 4)
      @object.send(@method, '79+4j').should == Complex(79, 4)
      @object.send(@method, '79+4J').should == Complex(79, 4)
    end
  end

  it "understands scientific notation for the real part" do
    NATFIXME 'understands scientific notation for the real part', exception: SpecFailedException do
      @object.send(@method, '2e3+4i').should == Complex(2e3,4)
    end
  end

  it "understands negative scientific notation for the real part" do
    NATFIXME 'understands negative scientific notation for the real part', exception: SpecFailedException do
      @object.send(@method, '-2e3+4i').should == Complex(-2e3,4)
    end
  end

  it "understands scientific notation for the imaginary part" do
    NATFIXME 'understands scientific notation for the imaginary part', exception: SpecFailedException do
      @object.send(@method, '4+2e3i').should == Complex(4, 2e3)
    end
  end

  it "understands negative scientific notation for the imaginary part" do
    NATFIXME 'understands negative scientific notation for the imaginary part', exception: SpecFailedException do
      @object.send(@method, '4-2e3i').should == Complex(4, -2e3)
    end
  end

  it "understands scientific notation for the real and imaginary part in the same String" do
    NATFIXME 'understands scientific notation for the real and imaginary part in the same String', exception: SpecFailedException do
      @object.send(@method, '2e3+2e4i').should == Complex(2e3,2e4)
    end
  end

  it "understands negative scientific notation for the real and imaginary part in the same String" do
    NATFIXME 'understands negative scientific notation for the real and imaginary part in the same String', exception: SpecFailedException do
      @object.send(@method, '-2e3-2e4i').should == Complex(-2e3,-2e4)
    end
  end

  it "understands scientific notation with e and E" do
    NATFIXME 'understands scientific notation with e and E', exception: SpecFailedException do
      @object.send(@method, '2e3+2e4i').should == Complex(2e3, 2e4)
      @object.send(@method, '2E3+2E4i').should == Complex(2e3, 2e4)
    end
  end

  it "understands 'm@a' to mean a complex number in polar form with 'm' as the modulus, 'a' as the argument" do
    NATFIXME "understands 'm@a' to mean a complex number in polar form with 'm' as the modulus, 'a' as the argument", exception: SpecFailedException do
      @object.send(@method, '79@4').should == Complex.polar(79, 4)
      @object.send(@method, '-79@4').should == Complex.polar(-79, 4)
      @object.send(@method, '79@-4').should == Complex.polar(79, -4)
    end
  end

  it "ignores leading whitespaces" do
    NATFIXME 'ignores leading whitespaces', exception: SpecFailedException do
      @object.send(@method, '  79+4i').should == Complex(79, 4)
    end
  end

  it "ignores trailing whitespaces" do
    NATFIXME 'ignores trailing whitespaces', exception: SpecFailedException do
      @object.send(@method, '79+4i  ').should == Complex(79, 4)
    end
  end

  it "understands _" do
    NATFIXME 'understands _', exception: SpecFailedException do
      @object.send(@method, '7_9+4_0i').should == Complex(79, 40)
    end
  end
end
