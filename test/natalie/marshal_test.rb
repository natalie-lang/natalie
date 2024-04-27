require_relative '../spec_helper'

describe 'Marshal' do
  describe '.dump' do
    it 'writes nil' do
      Marshal.dump(nil).should == "\x04\b0".force_encoding(Encoding::BINARY)
    end

    it 'writes true' do
      Marshal.dump(true).should == "\x04\bT".force_encoding(Encoding::BINARY)
    end

    it 'writes false' do
      Marshal.dump(false).should == "\x04\bF".force_encoding(Encoding::BINARY)
    end

    it 'writes integers' do
      Marshal.dump(0).should == "\x04\bi\x00".force_encoding(Encoding::BINARY)
      Marshal.dump(4).should == "\x04\bi\t".force_encoding(Encoding::BINARY)
      Marshal.dump(128).should == "\x04\bi\x01\x80".force_encoding(Encoding::BINARY)
      Marshal.dump(123456).should == "\x04\bi\x03@\xE2\x01".force_encoding(Encoding::BINARY)
      Marshal.dump(-1).should == "\x04\bi\xFA".force_encoding(Encoding::BINARY)
      Marshal.dump(-4).should == "\x04\bi\xF7".force_encoding(Encoding::BINARY)
      Marshal.dump(-128).should == "\x04\bi\xFF\x80".force_encoding(Encoding::BINARY)
      Marshal.dump(-123456).should == "\x04\bi\xFD\xC0\x1D\xFE".force_encoding(Encoding::BINARY)
    end

    it 'writes strings' do
      Marshal.dump('').should == "\x04\bI\"\x00\x06:\x06ET".force_encoding(Encoding::BINARY)
      Marshal.dump('ruby').should == "\x04\bI\"\truby\x06:\x06ET".force_encoding(Encoding::BINARY)
    end

    it 'writes symbols' do
      Marshal.dump(:ruby).should == "\x04\b:\truby".force_encoding(Encoding::BINARY)
      Marshal.dump(:E).should == "\x04\b:\x06E".force_encoding(Encoding::BINARY)
    end

    it 'writes floats' do
      Marshal.dump(1.23).should == "\x04\bf\t1.23".force_encoding(Encoding::BINARY)
      Marshal.dump(-1.23).should == "\x04\bf\n-1.23".force_encoding(Encoding::BINARY)
    end

    it 'writes arrays' do
      Marshal.dump([]).should == "\x04\b[\x00".force_encoding(Encoding::BINARY)
      Marshal.dump([1, 2, 3]).should == "\x04\b[\bi\x06i\ai\b".force_encoding(Encoding::BINARY)
    end

    it 'writes hashes' do
      Marshal.dump({}).should == "\x04\b{\x00".force_encoding(Encoding::BINARY)
      Marshal.dump({a: 1}).should == "\x04\b{\x06:\x06ai\x06".force_encoding(Encoding::BINARY)
      Marshal.dump({E: true}).should == "\x04\b{\x06:\x06ET".force_encoding(Encoding::BINARY)
    end

    it 'writes classes' do
      Marshal.dump(Array).should == "\x04\bc\nArray".force_encoding(Encoding::BINARY)
    end

    it 'writes modules' do
      Marshal.dump(Enumerable).should == "\x04\bm\x0FEnumerable".force_encoding(Encoding::BINARY)
    end

    it 'writes objects' do
      object = Object.new
      object.instance_variable_set(:@ivar, 1)
      Marshal.dump(object).should == "\x04\bo:\vObject\x06:\n@ivari\x06"
    end

    it 'writes complex numbers' do
      Marshal.dump(Complex(1, 2)).should == "\x04\bU:\fComplex[\ai\x06i\a".force_encoding(Encoding::BINARY)
    end

    it 'writes rational numbers' do
      Marshal.dump(Rational(1, 2)).should == "\x04\bU:\rRational[\ai\x06i\a".force_encoding(Encoding::BINARY)
    end
  end

  describe '.load' do
    it 'reads nil' do
      Marshal.load("\x04\b0".force_encoding(Encoding::BINARY)).should == nil
    end

    it 'reads true' do
      Marshal.load("\x04\bT".force_encoding(Encoding::BINARY)).should == true
    end

    it 'reads false' do
      Marshal.load("\x04\bF".force_encoding(Encoding::BINARY)).should == false
    end

    it 'reads integers' do
      Marshal.load("\x04\bi\x00".force_encoding(Encoding::BINARY)).should == 0
      Marshal.load("\x04\bi\t".force_encoding(Encoding::BINARY)).should == 4
      Marshal.load("\x04\bi\x01\x80".force_encoding(Encoding::BINARY)).should == 128
      Marshal.load("\x04\bi\x03@\xE2\x01".force_encoding(Encoding::BINARY)).should == 123456
      Marshal.load("\x04\bi\xFA".force_encoding(Encoding::BINARY)).should == -1
      Marshal.load("\x04\bi\xF7".force_encoding(Encoding::BINARY)).should == -4
      Marshal.load("\x04\bi\xFF\x80".force_encoding(Encoding::BINARY)).should == -128
      Marshal.load("\x04\bi\xFD\xC0\x1D\xFE".force_encoding(Encoding::BINARY)).should == -123456
    end

    it 'reads strings' do
      Marshal.load("\x04\bI\"\x00\x06:\x06ET".force_encoding(Encoding::BINARY)).should == ''
      Marshal.load("\x04\bI\"\truby\x06:\x06ET".force_encoding(Encoding::BINARY)).should == 'ruby'
    end

    it 'reads symbols' do
      Marshal.load("\x04\b:\truby".force_encoding(Encoding::BINARY)).should == :ruby
      Marshal.load("\x04\b:\x06E".force_encoding(Encoding::BINARY)).should == :E
    end

    it 'reads floats' do
      Marshal.load("\x04\bf\t1.23".force_encoding(Encoding::BINARY)).should == 1.23
      Marshal.load("\x04\bf\n-1.23".force_encoding(Encoding::BINARY)).should == -1.23
    end

    it 'reads arrays' do
      Marshal.load("\x04\b[\x00".force_encoding(Encoding::BINARY)).should == []
      Marshal.load("\x04\b[\bi\x06i\ai\b".force_encoding(Encoding::BINARY)).should == [1, 2, 3]
    end

    it 'reads hashes' do
      Marshal.load("\x04\b{\x00".force_encoding(Encoding::BINARY)).should == {}
      Marshal.load("\x04\b{\x06:\x06ai\x06".force_encoding(Encoding::BINARY)).should == {a: 1}
      Marshal.load("\x04\b{\x06:\x06ET".force_encoding(Encoding::BINARY)).should == {E: true}
    end

    it 'reads hashes with default values' do
      Marshal.load("\x04\b}\x00i\x06".force_encoding(Encoding::BINARY)).default.should == 1
      Marshal.load("\x04\b}\x06:\x06ai\x06i\x06".force_encoding(Encoding::BINARY)).default.should == 1
      Marshal.load("\x04\b}\x06:\x06ETi\x06".force_encoding(Encoding::BINARY)).default.should == 1
    end

    it 'reads classes' do
      Marshal.load("\x04\bc\nArray".force_encoding(Encoding::BINARY)).should == Array
      ->{ Marshal.load("\x04\bc\nArraz") }.should raise_error(ArgumentError)
      ->{ Marshal.load("\x04\bc\x0FEnumerable") }.should raise_error(ArgumentError)
    end

    it 'reads modules' do
      Marshal.load("\x04\bm\x0FEnumerable".force_encoding(Encoding::BINARY)).should == Enumerable
      ->{ Marshal.load("\x04\bm\x0FEnumerablz") }.should raise_error(ArgumentError)
      ->{ Marshal.load("\x04\bm\nArray") }.should raise_error(ArgumentError)
    end

    it 'reads regexps' do
      Marshal.load("\x04\bI/\truby\x00\x06:\x06EF".force_encoding(Encoding::BINARY)).should == /ruby/
      Marshal.load("\x04\bI/\truby\x01\x06:\x06EF".force_encoding(Encoding::BINARY)).should == /ruby/i
    end

    it 'reads objects' do
      object = Marshal.load("\x04\bo:\vObject\x06:\n@ivari\x06".force_encoding(Encoding::BINARY))
      object.should be_an_instance_of(Object)
      object.instance_variable_get(:@ivar).should == 1
    end

    it 'reads complex numbers' do
      Marshal.load("\x04\bU:\fComplex[\ai\x06i\a".force_encoding(Encoding::BINARY)).should == Complex(1, 2)
    end

    it 'reads rational numbers' do
      Marshal.load("\x04\bU:\rRational[\ai\x06i\a".force_encoding(Encoding::BINARY)).should == Rational(1, 2)
    end
  end
end
