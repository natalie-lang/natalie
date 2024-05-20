# frozen_string_literal: true

require_relative '../spec_helper'

describe 'Marshal' do
  describe '.dump' do
    it 'writes nil' do
      Marshal.dump(nil).should == "\x04\b0".b
    end

    it 'writes true' do
      Marshal.dump(true).should == "\x04\bT".b
    end

    it 'writes false' do
      Marshal.dump(false).should == "\x04\bF".b
    end

    it 'writes integers' do
      Marshal.dump(0).should == "\x04\bi\x00".b
      Marshal.dump(4).should == "\x04\bi\t".b
      Marshal.dump(128).should == "\x04\bi\x01\x80".b
      Marshal.dump(123456).should == "\x04\bi\x03@\xE2\x01".b
      Marshal.dump(-1).should == "\x04\bi\xFA".b
      Marshal.dump(-4).should == "\x04\bi\xF7".b
      Marshal.dump(-128).should == "\x04\bi\xFF\x80".b
      Marshal.dump(-123456).should == "\x04\bi\xFD\xC0\x1D\xFE".b
    end

    it 'writes strings' do
      Marshal.dump('').should == "\x04\bI\"\x00\x06:\x06ET".b
      Marshal.dump('ruby').should == "\x04\bI\"\truby\x06:\x06ET".b
    end

    it 'writes symbols' do
      Marshal.dump(:ruby).should == "\x04\b:\truby".b
      Marshal.dump(:E).should == "\x04\b:\x06E".b
    end

    it 'writes floats' do
      Marshal.dump(1.23).should == "\x04\bf\t1.23".b
      Marshal.dump(-1.23).should == "\x04\bf\n-1.23".b
    end

    it 'writes arrays' do
      Marshal.dump([]).should == "\x04\b[\x00".b
      Marshal.dump([1, 2, 3]).should == "\x04\b[\bi\x06i\ai\b".b
    end

    it 'writes hashes' do
      Marshal.dump({}).should == "\x04\b{\x00".b
      Marshal.dump({a: 1}).should == "\x04\b{\x06:\x06ai\x06".b
      Marshal.dump({E: true}).should == "\x04\b{\x06:\x06ET".b
    end

    it 'writes classes' do
      Marshal.dump(Array).should == "\x04\bc\nArray".b
    end

    it 'writes modules' do
      Marshal.dump(Enumerable).should == "\x04\bm\x0FEnumerable".b
    end

    it 'writes objects' do
      object = Object.new
      object.instance_variable_set(:@ivar, 1)
      Marshal.dump(object).should == "\x04\bo:\vObject\x06:\n@ivari\x06".b
    end

    it 'writes complex numbers' do
      Marshal.dump(Complex(1, 2)).should == "\x04\bU:\fComplex[\ai\x06i\a".b
    end

    it 'writes rational numbers' do
      Marshal.dump(Rational(1, 2)).should == "\x04\bU:\rRational[\ai\x06i\a".b
    end
  end

  describe '.load' do
    it 'reads nil' do
      Marshal.load("\x04\b0".b).should == nil
    end

    it 'reads true' do
      Marshal.load("\x04\bT".b).should == true
    end

    it 'reads false' do
      Marshal.load("\x04\bF".b).should == false
    end

    it 'reads integers' do
      Marshal.load("\x04\bi\x00".b).should == 0
      Marshal.load("\x04\bi\t".b).should == 4
      Marshal.load("\x04\bi\x01\x80".b).should == 128
      Marshal.load("\x04\bi\x03@\xE2\x01".b).should == 123456
      Marshal.load("\x04\bi\xFA".b).should == -1
      Marshal.load("\x04\bi\xF7".b).should == -4
      Marshal.load("\x04\bi\xFF\x80".b).should == -128
      Marshal.load("\x04\bi\xFD\xC0\x1D\xFE".b).should == -123456
    end

    it 'reads strings' do
      Marshal.load("\x04\bI\"\x00\x06:\x06ET".b).should == ''
      Marshal.load("\x04\bI\"\truby\x06:\x06ET".b).should == 'ruby'
    end

    it 'reads symbols' do
      Marshal.load("\x04\b:\truby".b).should == :ruby
      Marshal.load("\x04\b:\x06E".b).should == :E
    end

    it 'reads floats' do
      Marshal.load("\x04\bf\t1.23".b).should == 1.23
      Marshal.load("\x04\bf\n-1.23".b).should == -1.23
    end

    it 'reads arrays' do
      Marshal.load("\x04\b[\x00".b).should == []
      Marshal.load("\x04\b[\bi\x06i\ai\b".b).should == [1, 2, 3]
    end

    it 'reads hashes' do
      Marshal.load("\x04\b{\x00".b).should == {}
      Marshal.load("\x04\b{\x06:\x06ai\x06".b).should == {a: 1}
      Marshal.load("\x04\b{\x06:\x06ET".b).should == {E: true}
    end

    it 'reads hashes with default values' do
      Marshal.load("\x04\b}\x00i\x06".b).default.should == 1
      Marshal.load("\x04\b}\x06:\x06ai\x06i\x06".b).default.should == 1
      Marshal.load("\x04\b}\x06:\x06ETi\x06".b).default.should == 1
    end

    it 'reads classes' do
      Marshal.load("\x04\bc\nArray".b).should == Array
      ->{ Marshal.load("\x04\bc\nArraz".b) }.should raise_error(ArgumentError)
      ->{ Marshal.load("\x04\bc\x0FEnumerable".b) }.should raise_error(ArgumentError)
    end

    it 'reads modules' do
      Marshal.load("\x04\bm\x0FEnumerable".b).should == Enumerable
      ->{ Marshal.load("\x04\bm\x0FEnumerablz".b) }.should raise_error(ArgumentError)
      ->{ Marshal.load("\x04\bm\nArray".b) }.should raise_error(ArgumentError)
    end

    it 'reads regexps' do
      Marshal.load("\x04\bI/\truby\x00\x06:\x06EF".b).should == /ruby/
      Marshal.load("\x04\bI/\truby\x01\x06:\x06EF".b).should == /ruby/i
    end

    it 'reads objects' do
      object = Marshal.load("\x04\bo:\vObject\x06:\n@ivari\x06".b)
      object.should be_an_instance_of(Object)
      object.instance_variable_get(:@ivar).should == 1
    end

    it 'reads complex numbers' do
      Marshal.load("\x04\bU:\fComplex[\ai\x06i\a".b).should == Complex(1, 2)
    end

    it 'reads rational numbers' do
      Marshal.load("\x04\bU:\rRational[\ai\x06i\a".b).should == Rational(1, 2)
    end
  end
end
