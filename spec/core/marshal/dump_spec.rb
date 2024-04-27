# -*- encoding: binary -*-
require_relative '../../spec_helper'
require_relative 'fixtures/classes'
require_relative 'fixtures/marshal_data'

describe "Marshal.dump" do
  it "dumps nil" do
    Marshal.dump(nil).should == "\004\b0"
  end

  it "dumps true" do
    Marshal.dump(true).should == "\004\bT"
  end

  it "dumps false" do
    Marshal.dump(false).should == "\004\bF"
  end

  describe "with a Fixnum" do
    it "dumps a Fixnum" do
      [ [Marshal,  0,       "\004\bi\000"],
        [Marshal,  5,       "\004\bi\n"],
        [Marshal,  8,       "\004\bi\r"],
        [Marshal,  122,     "\004\bi\177"],
        [Marshal,  123,     "\004\bi\001{"],
        [Marshal,  1234,    "\004\bi\002\322\004"],
        [Marshal, -8,       "\004\bi\363"],
        [Marshal, -123,     "\004\bi\200"],
        [Marshal, -124,     "\004\bi\377\204"],
        [Marshal, -1234,    "\004\bi\376.\373"],
        [Marshal, -4516727, "\004\bi\375\211\024\273"],
        [Marshal,  2**8,    "\004\bi\002\000\001"],
        [Marshal,  2**16,   "\004\bi\003\000\000\001"],
        [Marshal,  2**24,   "\004\bi\004\000\000\000\001"],
        [Marshal, -2**8,    "\004\bi\377\000"],
        [Marshal, -2**16,   "\004\bi\376\000\000"],
        [Marshal, -2**24,   "\004\bi\375\000\000\000"],
      ].should be_computed_by(:dump)
    end

    platform_is wordsize: 64 do
      it "dumps a positive Fixnum > 31 bits as a Bignum" do
        Marshal.dump(2**31 + 1).should == "\x04\bl+\a\x01\x00\x00\x80"
      end

      it "dumps a negative Fixnum > 31 bits as a Bignum" do
        Marshal.dump(-2**31 - 1).should == "\x04\bl-\a\x01\x00\x00\x80"
      end
    end
  end

  describe "with a Symbol" do
    it "dumps a Symbol" do
      Marshal.dump(:symbol).should == "\004\b:\vsymbol"
    end

    it "dumps a big Symbol" do
      Marshal.dump(('big' * 100).to_sym).should == "\004\b:\002,\001#{'big' * 100}"
    end

    it "dumps an encoded Symbol" do
      s = "\u2192"
      NATFIXME 'Fix encoding of result and implement UTF-16 encoding', exception: ArgumentError, message: 'unknown encoding name - "utf-16"' do
        [ [Marshal, s.encode("utf-8").to_sym,
              "\x04\bI:\b\xE2\x86\x92\x06:\x06ET"],
          [Marshal, s.encode("utf-16").to_sym,
              "\x04\bI:\t\xFE\xFF!\x92\x06:\rencoding\"\vUTF-16"],
          [Marshal, s.encode("utf-16le").to_sym,
              "\x04\bI:\a\x92!\x06:\rencoding\"\rUTF-16LE"],
          [Marshal, s.encode("utf-16be").to_sym,
              "\x04\bI:\a!\x92\x06:\rencoding\"\rUTF-16BE"],
          [Marshal, s.encode("euc-jp").to_sym,
              "\x04\bI:\a\xA2\xAA\x06:\rencoding\"\vEUC-JP"],
          [Marshal, s.encode("sjis").to_sym,
              "\x04\bI:\a\x81\xA8\x06:\rencoding\"\x10Windows-31J"]
        ].should be_computed_by(:dump)
      end
    end

    it "dumps a binary encoded Symbol" do
      s = "\u2192".dup.force_encoding("binary").to_sym
      NATFIXME 'dumps a binary encoded Symbol', exception: SpecFailedException do
        Marshal.dump(s).should == "\x04\b:\b\xE2\x86\x92"
      end
    end

    it "dumps multiple Symbols sharing the same encoding" do
      # Note that the encoding is a link for the second Symbol
      symbol1 = "I:\t\xE2\x82\xACa\x06:\x06ET"
      symbol2 = "I:\t\xE2\x82\xACb\x06;\x06T"
      value = [
        "€a".dup.force_encoding(Encoding::UTF_8).to_sym,
        "€b".dup.force_encoding(Encoding::UTF_8).to_sym
      ]
      NATFIXME 'Encoding of output', exception: Encoding::CompatibilityError, message: 'incompatible character encodings: ASCII-8BIT and UTF-8' do
        Marshal.dump(value).should == "\x04\b[\a#{symbol1}#{symbol2}"

        value = [*value, value[0]]
        Marshal.dump(value).should == "\x04\b[\b#{symbol1}#{symbol2};\x00"
      end
    end
  end

  describe "with an object responding to #marshal_dump" do
    it "dumps the object returned by #marshal_dump" do
      Marshal.dump(UserMarshal.new).should == "\x04\bU:\x10UserMarshal:\tdata"
    end

    it "does not use Class#name" do
      NATFIXME 'does not use Class#name', exception: SpecFailedException do
        UserMarshal.should_not_receive(:name)
        Marshal.dump(UserMarshal.new)
      end
    end

    it "raises TypeError if an Object is an instance of an anonymous class" do
      NATFIXME 'raises TypeError if an Object is an instance of an anonymous class', exception: SpecFailedException do
        -> { Marshal.dump(Class.new(UserMarshal).new) }.should raise_error(TypeError, /can't dump anonymous class/)
      end
    end
  end

  describe "with an object responding to #_dump" do
    it "dumps the String returned by #_dump" do
      NATFIXME 'dumps the String returned by #_dump', exception: SpecFailedException do
        Marshal.dump(UserDefined.new).should == "\004\bu:\020UserDefined\022\004\b[\a:\nstuff;\000"
      end
    end

    it "dumps the String in non US-ASCII and non UTF-8 encoding" do
      object = UserDefinedString.new("a".encode("windows-1251"))
      NATFIXME 'dumps the String in non US-ASCII and non UTF-8 encoding', exception: SpecFailedException do
        Marshal.dump(object).should == "\x04\bIu:\x16UserDefinedString\x06a\x06:\rencoding\"\x11Windows-1251"
      end
    end

    it "dumps the String in multibyte encoding" do
      object = UserDefinedString.new("a".encode("utf-32le"))
      NATFIXME 'Encoding of output', exception: Encoding::CompatibilityError, message: 'incompatible character encodings: ASCII-8BIT and UTF-32LE' do
        Marshal.dump(object).should == "\x04\bIu:\x16UserDefinedString\ta\x00\x00\x00\x06:\rencoding\"\rUTF-32LE"
      end
    end

    it "ignores overridden name method" do
      obj = MarshalSpec::UserDefinedWithOverriddenName.new
      NATFIXME 'ignores overridden name method', exception: SpecFailedException do
        Marshal.dump(obj).should == "\x04\bu:/MarshalSpec::UserDefinedWithOverriddenName\x12\x04\b[\a:\nstuff;\x00"
      end
    end

    it "raises a TypeError if _dump returns a non-string" do
      m = mock("marshaled")
      NATFIXME 'raises a TypeError if _dump returns a non-string', exception: SpecFailedException do
        m.should_receive(:_dump).and_return(0)
        -> { Marshal.dump(m) }.should raise_error(TypeError)
      end
    end

    it "raises TypeError if an Object is an instance of an anonymous class" do
      -> { Marshal.dump(Class.new(UserDefined).new) }.should raise_error(TypeError, /can't dump anonymous class/)
    end

    it "favors marshal_dump over _dump" do
      m = mock("marshaled")
      m.should_receive(:marshal_dump).and_return(0)
      m.should_not_receive(:_dump)
      Marshal.dump(m)
    end

    it "indexes instance variables of a String returned by #_dump at first and then indexes the object itself" do
      class MarshalSpec::M1::A
        def _dump(level)
          s = +"<dump>"
          s.instance_variable_set(:@foo, "bar")
          s
        end
      end

      a = MarshalSpec::M1::A.new

      # 0-based index of the object a = 2, that is encoded as \x07 and printed as "\a" character.
      # Objects are serialized in the following order: Array, a, "bar".
      # But they are indexed in different order: Array (index=0), "bar" (index=1), a (index=2)
      # So the second occurenc of the object a is encoded as an index 2.
      reference = "@\a"
      NATFIXME 'indexes instance variables of a String returned by #_dump at first and then indexes the object itself', exception: SpecFailedException do
        Marshal.dump([a, a]).should == "\x04\b[\aIu:\x17MarshalSpec::M1::A\v<dump>\x06:\t@foo\"\bbar#{reference}"
      end
    end

    describe "Core library classes with #_dump returning a String with instance variables" do
      it "indexes instance variables and then a Time object itself" do
        t = Time.utc(2022)
        reference = "@\a"

        NATFIXME 'indexes instance variables and then a Time object itself', exception: SpecFailedException do
          Marshal.dump([t, t]).should == "\x04\b[\aIu:\tTime\r \x80\x1E\xC0\x00\x00\x00\x00\x06:\tzoneI\"\bUTC\x06:\x06EF#{reference}"
        end
      end
    end
  end

  describe "with a Class" do
    it "dumps a builtin Class" do
      Marshal.dump(String).should == "\004\bc\vString"
    end

    it "dumps a user Class" do
      Marshal.dump(UserDefined).should == "\x04\bc\x10UserDefined"
    end

    it "dumps a nested Class" do
      Marshal.dump(UserDefined::Nested).should == "\004\bc\030UserDefined::Nested"
    end

    it "ignores overridden name method" do
      NATFIXME 'ignores overridden name method', exception: SpecFailedException do
        Marshal.dump(MarshalSpec::ClassWithOverriddenName).should == "\x04\bc)MarshalSpec::ClassWithOverriddenName"
      end
    end

    it "dumps a class with multibyte characters in name" do
      NATFIXME 'eval', exception: TypeError, message: 'eval() only works on static strings' do
        source_object = eval("MarshalSpec::MultibyteぁあぃいClass".dup.force_encoding(Encoding::UTF_8))
        Marshal.dump(source_object).should == "\x04\bc,MarshalSpec::Multibyte\xE3\x81\x81\xE3\x81\x82\xE3\x81\x83\xE3\x81\x84Class"
      end
    end

    it "raises TypeError with an anonymous Class" do
      NATFIXME 'raises TypeError with an anonymous Class', exception: SpecFailedException do
        -> { Marshal.dump(Class.new) }.should raise_error(TypeError, /can't dump anonymous class/)
      end
    end

    it "raises TypeError with a singleton Class" do
      NATFIXME 'raises TypeError with a singleton Class', exception: SpecFailedException do
        -> { Marshal.dump(class << self; self end) }.should raise_error(TypeError)
      end
    end
  end

  describe "with a Module" do
    it "dumps a builtin Module" do
      Marshal.dump(Marshal).should == "\004\bm\fMarshal"
    end

    it "ignores overridden name method" do
      NATFIXME 'ignores overridden name method', exception: SpecFailedException do
        Marshal.dump(MarshalSpec::ModuleWithOverriddenName).should == "\x04\bc*MarshalSpec::ModuleWithOverriddenName"
      end
    end

    it "dumps a module with multibyte characters in name" do
      NATFIXME 'eval', exception: TypeError, message: 'eval() only works on static strings' do
        source_object = eval("MarshalSpec::MultibyteけげこごModule".dup.force_encoding(Encoding::UTF_8))
        Marshal.dump(source_object).should == "\x04\bm-MarshalSpec::Multibyte\xE3\x81\x91\xE3\x81\x92\xE3\x81\x93\xE3\x81\x94Module"
      end
    end

    it "raises TypeError with an anonymous Module" do
      NATFIXME 'raises TypeError with an anonymous Module', exception: SpecFailedException do
        -> { Marshal.dump(Module.new) }.should raise_error(TypeError, /can't dump anonymous module/)
      end
    end
  end

  describe "with a Float" do
    it "dumps a Float" do
      [ [Marshal,  0.0,            "\004\bf\0060"],
        [Marshal, -0.0,            "\004\bf\a-0"],
        [Marshal,  1.0,            "\004\bf\0061"],
        [Marshal,  123.4567,       "\004\bf\r123.4567"],
        [Marshal, -0.841,          "\x04\bf\v-0.841"],
        [Marshal, -9876.345,       "\x04\bf\x0E-9876.345"],
        [Marshal,  infinity_value, "\004\bf\binf"],
        [Marshal, -infinity_value, "\004\bf\t-inf"],
        [Marshal,  nan_value,      "\004\bf\bnan"],
      ].should be_computed_by(:dump)
    end
  end

  describe "with a Bignum" do
    it "dumps a Bignum" do
      [ [Marshal, -4611686018427387903,    "\004\bl-\t\377\377\377\377\377\377\377?"],
        [Marshal, -2361183241434822606847, "\004\bl-\n\377\377\377\377\377\377\377\377\177\000"],
      ].should be_computed_by(:dump)
    end

    it "dumps a Bignum" do
      [ [Marshal,  2**64, "\004\bl+\n\000\000\000\000\000\000\000\000\001\000"],
        [Marshal,  2**90, "\004\bl+\v#{"\000" * 11}\004"],
        [Marshal, -2**63, "\004\bl-\t\000\000\000\000\000\000\000\200"],
        [Marshal, -2**64, "\004\bl-\n\000\000\000\000\000\000\000\000\001\000"],
      ].should be_computed_by(:dump)
    end

    it "increases the object links counter" do
      obj = Object.new
      object_1_link = "\x06" # representing of (0-based) index=1 (by adding 5 for small Integers)
      object_2_link = "\x07" # representing of index=2

      # objects: Array, Object, Object
      NATFIXME 'increases the object links counter', exception: SpecFailedException do
        Marshal.dump([obj, obj]).should == "\x04\b[\ao:\vObject\x00@#{object_1_link}"

        # objects: Array, Bignum, Object, Object
        Marshal.dump([2**64, obj, obj]).should == "\x04\b[\bl+\n\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00o:\vObject\x00@#{object_2_link}"
        Marshal.dump([2**48, obj, obj]).should == "\x04\b[\bl+\t\x00\x00\x00\x00\x00\x00\x01\x00o:\vObject\x00@#{object_2_link}"
        Marshal.dump([2**32, obj, obj]).should == "\x04\b[\bl+\b\x00\x00\x00\x00\x01\x00o:\vObject\x00@#{object_2_link}"
      end
    end
  end

  describe "with a Rational" do
    it "dumps a Rational" do
      Marshal.dump(Rational(2, 3)).should == "\x04\bU:\rRational[\ai\ai\b"
    end
  end

  describe "with a Complex" do
    it "dumps a Complex" do
      Marshal.dump(Complex(2, 3)).should == "\x04\bU:\fComplex[\ai\ai\b"
    end
  end

  describe "with a String" do
    it "dumps a blank String" do
      NATFIXME 'dumps a blank String', exception: SpecFailedException do
        Marshal.dump("".dup.force_encoding("binary")).should == "\004\b\"\000"
      end
    end

    it "dumps a short String" do
      NATFIXME 'dumps a short String', exception: SpecFailedException do
        Marshal.dump("short".dup.force_encoding("binary")).should == "\004\b\"\012short"
      end
    end

    it "dumps a long String" do
      NATFIXME 'dumps a long String', exception: SpecFailedException do
        Marshal.dump(("big" * 100).force_encoding("binary")).should == "\004\b\"\002,\001#{"big" * 100}"
      end
    end

    it "dumps a String extended with a Module" do
      NATFIXME 'dumps a String extended with a Module', exception: SpecFailedException do
        Marshal.dump("".dup.extend(Meths).force_encoding("binary")).should == "\004\be:\nMeths\"\000"
      end
    end

    it "dumps a String subclass" do
      NATFIXME 'dumps a String subclass', exception: SpecFailedException do
        Marshal.dump(UserString.new.force_encoding("binary")).should == "\004\bC:\017UserString\"\000"
      end
    end

    it "dumps a String subclass extended with a Module" do
      NATFIXME 'dumps a String subclass extended with a Module', exception: SpecFailedException do
        Marshal.dump(UserString.new.extend(Meths).force_encoding("binary")).should == "\004\be:\nMethsC:\017UserString\"\000"
      end
    end

    it "ignores overridden name method when dumps a String subclass" do
      obj = MarshalSpec::StringWithOverriddenName.new
      NATFIXME 'ignores overridden name method when dumps a String subclass', exception: SpecFailedException do
        Marshal.dump(obj).should == "\x04\bC:*MarshalSpec::StringWithOverriddenName\"\x00"
      end
    end

    it "dumps a String with instance variables" do
      str = +""
      str.instance_variable_set("@foo", "bar")
      NATFIXME 'dumps a String with instance variables', exception: SpecFailedException do
        Marshal.dump(str.force_encoding("binary")).should == "\x04\bI\"\x00\x06:\t@foo\"\bbar"
      end
    end

    it "dumps a US-ASCII String" do
      str = "abc".dup.force_encoding("us-ascii")
      Marshal.dump(str).should == "\x04\bI\"\babc\x06:\x06EF"
    end

    it "dumps a UTF-8 String" do
      str = "\x6d\xc3\xb6\x68\x72\x65".dup.force_encoding("utf-8")
      NATFIXME 'dumps a UTF-8 String', exception: SpecFailedException do
        Marshal.dump(str).should == "\x04\bI\"\vm\xC3\xB6hre\x06:\x06ET"
      end
    end

    it "dumps a String in another encoding" do
      str = "\x6d\x00\xf6\x00\x68\x00\x72\x00\x65\x00".dup.force_encoding("utf-16le")
      result = "\x04\bI\"\x0Fm\x00\xF6\x00h\x00r\x00e\x00\x06:\rencoding\"\rUTF-16LE"
      NATFIXME 'String encoding of result', exception: Encoding::CompatibilityError, message: 'incompatible character encodings: ASCII-8BIT and UTF-16LE' do
        Marshal.dump(str).should == result
      end
    end

    it "dumps multiple strings using symlinks for the :E (encoding) symbol" do
      Marshal.dump(["".encode("us-ascii"), "".encode("utf-8")]).should == "\x04\b[\aI\"\x00\x06:\x06EFI\"\x00\x06;\x00T"
    end
  end

  describe "with a Regexp" do
    it "dumps a Regexp" do
      NATFIXME 'dumps a Regexp', exception: SpecFailedException do
        Marshal.dump(/\A.\Z/).should == "\x04\bI/\n\\A.\\Z\x00\x06:\x06EF"
      end
    end

    it "dumps a Regexp with flags" do
      NATFIXME 'dumps a Regexp with flags', exception: SpecFailedException do
        Marshal.dump(//im).should == "\x04\bI/\x00\x05\x06:\x06EF"
      end
    end

    it "dumps a Regexp with instance variables" do
      o = Regexp.new("")
      o.instance_variable_set(:@ivar, :ivar)
      NATFIXME 'dumps a Regexp with instance variables', exception: SpecFailedException do
        Marshal.dump(o).should == "\x04\bI/\x00\x00\a:\x06EF:\n@ivar:\tivar"
      end
    end

    it "dumps an extended Regexp" do
      NATFIXME 'dumps an extended Regexp', exception: SpecFailedException do
        Marshal.dump(Regexp.new("").extend(Meths)).should == "\x04\bIe:\nMeths/\x00\x00\x06:\x06EF"
      end
    end

    it "dumps a Regexp subclass" do
      NATFIXME 'dumps a Regexp subclass', exception: SpecFailedException do
        Marshal.dump(UserRegexp.new("")).should == "\x04\bIC:\x0FUserRegexp/\x00\x00\x06:\x06EF"
      end
    end

    it "dumps a binary Regexp" do
      o = Regexp.new("".dup.force_encoding("binary"), Regexp::FIXEDENCODING)
      NATFIXME 'dumps a binary Regexp', exception: SpecFailedException do
        Marshal.dump(o).should == "\x04\b/\x00\x10"
      end
    end

    it "dumps an ascii-compatible Regexp" do
      o = Regexp.new("a".encode("us-ascii"), Regexp::FIXEDENCODING)
      NATFIXME 'dumps an ascii-compatible Regexp', exception: SpecFailedException do
        Marshal.dump(o).should == "\x04\bI/\x06a\x10\x06:\x06EF"

        o = Regexp.new("a".encode("us-ascii"))
        Marshal.dump(o).should == "\x04\bI/\x06a\x00\x06:\x06EF"

        o = Regexp.new("a".encode("windows-1251"), Regexp::FIXEDENCODING)
        Marshal.dump(o).should == "\x04\bI/\x06a\x10\x06:\rencoding\"\x11Windows-1251"

        o = Regexp.new("a".encode("windows-1251"))
        Marshal.dump(o).should == "\x04\bI/\x06a\x00\x06:\x06EF"
      end
    end

    it "dumps a UTF-8 Regexp" do
      o = Regexp.new("".dup.force_encoding("utf-8"), Regexp::FIXEDENCODING)
      NATFIXME 'dumps a UTF-8 Regexp', exception: SpecFailedException do
        Marshal.dump(o).should == "\x04\bI/\x00\x10\x06:\x06ET"

        o = Regexp.new("a".dup.force_encoding("utf-8"), Regexp::FIXEDENCODING)
        Marshal.dump(o).should == "\x04\bI/\x06a\x10\x06:\x06ET"

        o = Regexp.new("\u3042".dup.force_encoding("utf-8"), Regexp::FIXEDENCODING)
        Marshal.dump(o).should == "\x04\bI/\b\xE3\x81\x82\x10\x06:\x06ET"
      end
    end

    it "dumps a Regexp in another encoding" do
      o = Regexp.new("".dup.force_encoding("utf-16le"), Regexp::FIXEDENCODING)
      NATFIXME 'dumps a Regexp in another encoding', exception: SpecFailedException do
        Marshal.dump(o).should == "\x04\bI/\x00\x10\x06:\rencoding\"\rUTF-16LE"

        o = Regexp.new("a".encode("utf-16le"), Regexp::FIXEDENCODING)
        Marshal.dump(o).should == "\x04\bI/\aa\x00\x10\x06:\rencoding\"\rUTF-16LE"
      end
    end

    it "ignores overridden name method when dumps a Regexp subclass" do
      obj = MarshalSpec::RegexpWithOverriddenName.new("")
      NATFIXME 'ignores overridden name method when dumps a Regexp subclass', exception: SpecFailedException do
        Marshal.dump(obj).should == "\x04\bIC:*MarshalSpec::RegexpWithOverriddenName/\x00\x00\x06:\x06EF"
      end
    end
  end

  describe "with an Array" do
    it "dumps an empty Array" do
      Marshal.dump([]).should == "\004\b[\000"
    end

    it "dumps a non-empty Array" do
      Marshal.dump([:a, 1, 2]).should == "\004\b[\b:\006ai\006i\a"
    end

    it "dumps an Array subclass" do
      NATFIXME 'dumps an Array subclass', exception: SpecFailedException do
        Marshal.dump(UserArray.new).should == "\004\bC:\016UserArray[\000"
      end
    end

    # NATFIXME: This results in an infinite loop
    xit "dumps a recursive Array" do
      a = []
      a << a
      Marshal.dump(a).should == "\x04\b[\x06@\x00"
    end

    it "dumps an Array with instance variables" do
      a = []
      a.instance_variable_set(:@ivar, 1)
      NATFIXME 'dumps an Array with instance variables', exception: SpecFailedException do
        Marshal.dump(a).should == "\004\bI[\000\006:\n@ivari\006"
      end
    end

    it "dumps an extended Array" do
      NATFIXME 'dumps an extended Array', exception: SpecFailedException do
        Marshal.dump([].extend(Meths)).should == "\004\be:\nMeths[\000"
      end
    end

    it "ignores overridden name method when dumps an Array subclass" do
      obj = MarshalSpec::ArrayWithOverriddenName.new
      NATFIXME 'ignores overridden name method when dumps an Array subclass', exception: SpecFailedException do
        Marshal.dump(obj).should == "\x04\bC:)MarshalSpec::ArrayWithOverriddenName[\x00"
      end
    end
  end

  describe "with a Hash" do
    it "dumps a Hash" do
      Marshal.dump({}).should == "\004\b{\000"
    end

    it "dumps a non-empty Hash" do
      Marshal.dump({a: 1}).should == "\x04\b{\x06:\x06ai\x06"
    end

    it "dumps a Hash subclass" do
      NATFIXME 'dumps a Hash subclass', exception: SpecFailedException do
        Marshal.dump(UserHash.new).should == "\004\bC:\rUserHash{\000"
      end
    end

    it "dumps a Hash with a default value" do
      Marshal.dump(Hash.new(1)).should == "\004\b}\000i\006"
    end

    ruby_version_is "3.1" do
      it "dumps a Hash with compare_by_identity" do
        h = {}
        h.compare_by_identity

        NATFIXME 'dumps a Hash with compare_by_identity', exception: SpecFailedException do
          Marshal.dump(h).should == "\004\bC:\tHash{\x00"
        end
      end

      it "dumps a Hash subclass with compare_by_identity" do
        h = UserHash.new
        h.compare_by_identity

        NATFIXME 'dumps a Hash subclass with compare_by_identity', exception: SpecFailedException do
          Marshal.dump(h).should == "\x04\bC:\rUserHashC:\tHash{\x00"
        end
      end
    end

    it "raises a TypeError with hash having default proc" do
      NATFIXME 'raises a TypeError with hash having default proc', exception: SpecFailedException do
        -> { Marshal.dump(Hash.new {}) }.should raise_error(TypeError, "can't dump hash with default proc")
      end
    end

    it "dumps a Hash with instance variables" do
      a = {}
      a.instance_variable_set(:@ivar, 1)
      NATFIXME 'dumps a Hash with instance variables', exception: SpecFailedException do
        Marshal.dump(a).should == "\004\bI{\000\006:\n@ivari\006"
      end
    end

    it "dumps an extended Hash" do
      NATFIXME 'dumps an extended Hash', exception: SpecFailedException do
        Marshal.dump({}.extend(Meths)).should == "\004\be:\nMeths{\000"
      end
    end

    it "dumps an Hash subclass with a parameter to initialize" do
      NATFIXME 'dumps an Hash subclass with a parameter to initialize', exception: SpecFailedException do
        Marshal.dump(UserHashInitParams.new(1)).should == "\004\bIC:\027UserHashInitParams{\000\006:\a@ai\006"
      end
    end

    it "ignores overridden name method when dumps a Hash subclass" do
      obj = MarshalSpec::HashWithOverriddenName.new
      NATFIXME 'ignores overridden name method when dumps a Hash subclass', exception: SpecFailedException do
        Marshal.dump(obj).should == "\x04\bC:(MarshalSpec::HashWithOverriddenName{\x00"
      end
    end
  end

  describe "with a Struct" do
    it "dumps a Struct" do
      NATFIXME 'dumps a Struct', exception: SpecFailedException do
        Marshal.dump(Struct::Pyramid.new).should == "\004\bS:\024Struct::Pyramid\000"
      end
    end

    it "dumps a Struct" do
      NATFIXME 'dumps a Struct', exception: SpecFailedException do
        Marshal.dump(Struct::Useful.new(1, 2)).should == "\004\bS:\023Struct::Useful\a:\006ai\006:\006bi\a"
      end
    end

    it "dumps a Struct with instance variables" do
      st = Struct.new("Thick").new
      st.instance_variable_set(:@ivar, 1)
      NATFIXME 'dumps a Struct with instance variables', exception: SpecFailedException do
        Marshal.dump(st).should == "\004\bIS:\022Struct::Thick\000\006:\n@ivari\006"
      end
      Struct.send(:remove_const, :Thick)
    end

    it "dumps an extended Struct" do
      obj = Struct.new("Extended", :a, :b).new.extend(Meths)
      NATFIXME 'dumps an extended Struct', exception: SpecFailedException do
        Marshal.dump(obj).should == "\004\be:\nMethsS:\025Struct::Extended\a:\006a0:\006b0"

        s = 'hi'
        obj.a = [:a, s]
        obj.b = [:Meths, s]
        Marshal.dump(obj).should == "\004\be:\nMethsS:\025Struct::Extended\a:\006a[\a;\a\"\ahi:\006b[\a;\000@\a"
      end
      Struct.send(:remove_const, :Extended)
    end

    it "ignores overridden name method" do
      obj = MarshalSpec::StructWithOverriddenName.new("member")
      NATFIXME 'ignores overridden name method', exception: SpecFailedException do
        Marshal.dump(obj).should == "\x04\bS:*MarshalSpec::StructWithOverriddenName\x06:\x06a\"\vmember"
      end
    end

    it "raises TypeError with an anonymous Struct" do
      -> { Marshal.dump(Struct.new(:a).new(1)) }.should raise_error(TypeError, /can't dump anonymous class/)
    end
  end

  describe "with an Object" do
    it "dumps an Object" do
      Marshal.dump(Object.new).should == "\004\bo:\x0BObject\x00"
    end

    it "dumps an extended Object" do
      NATFIXME 'dumps an extended Object', exception: SpecFailedException do
        Marshal.dump(Object.new.extend(Meths)).should == "\004\be:\x0AMethso:\x0BObject\x00"
      end
    end

    it "dumps an Object with an instance variable" do
      obj = Object.new
      obj.instance_variable_set(:@ivar, 1)
      Marshal.dump(obj).should == "\004\bo:\vObject\006:\n@ivari\006"
    end

    it "dumps an Object with a non-US-ASCII instance variable" do
      obj = Object.new
      ivar = "@é".dup.force_encoding(Encoding::UTF_8).to_sym
      NATFIXME 'Support non-ASCII ivar names (seen NameError and Encoding::CompatibilityError)' do
        obj.instance_variable_set(ivar, 1)
        Marshal.dump(obj).should == "\x04\bo:\vObject\x06I:\b@\xC3\xA9\x06:\x06ETi\x06"
      end
    end

    it "dumps an Object that has had an instance variable added and removed as though it was never set" do
      obj = Object.new
      obj.instance_variable_set(:@ivar, 1)
      obj.send(:remove_instance_variable, :@ivar)
      Marshal.dump(obj).should == "\004\bo:\x0BObject\x00"
    end

    it "dumps an Object if it has a singleton class but no singleton methods and no singleton instance variables" do
      obj = Object.new
      obj.singleton_class
      Marshal.dump(obj).should == "\004\bo:\x0BObject\x00"
    end

    it "ignores overridden name method" do
      obj = MarshalSpec::ClassWithOverriddenName.new
      NATFIXME 'ignores overridden name method', exception: SpecFailedException do
        Marshal.dump(obj).should == "\x04\bo:)MarshalSpec::ClassWithOverriddenName\x00"
      end
    end

    it "raises TypeError if an Object has a singleton class and singleton methods" do
      obj = Object.new
      def obj.foo; end
      NATFIXME 'raises TypeError if an Object has a singleton class and singleton methods', exception: SpecFailedException do
        -> {
          Marshal.dump(obj)
        }.should raise_error(TypeError, "singleton can't be dumped")
      end
    end

    it "raises TypeError if an Object has a singleton class and singleton instance variables" do
      obj = Object.new
      class << obj
        @v = 1
      end

      NATFIXME 'raises TypeError if an Object has a singleton class and singleton instance variables', exception: SpecFailedException do
        -> {
          Marshal.dump(obj)
        }.should raise_error(TypeError, "singleton can't be dumped")
      end
    end

    it "raises TypeError if an Object is an instance of an anonymous class" do
      anonymous_class = Class.new
      obj = anonymous_class.new

      -> { Marshal.dump(obj) }.should raise_error(TypeError, /can't dump anonymous class/)
    end

    it "raises TypeError if an Object extends an anonymous module" do
      anonymous_module = Module.new
      obj = Object.new
      obj.extend(anonymous_module)

      NATFIXME 'raises TypeError if an Object extends an anonymous module', exception: SpecFailedException do
        -> { Marshal.dump(obj) }.should raise_error(TypeError, /can't dump anonymous class/)
      end
    end

    it "dumps a BasicObject subclass if it defines respond_to?" do
      obj = MarshalSpec::BasicObjectSubWithRespondToFalse.new
      NATFIXME 'it dumps a BasicObject subclass if it defines respond_to?', exception: NoMethodError, message: "undefined method `nil?' for an instance of MarshalSpec::BasicObjectSubWithRespondToFalse" do
        Marshal.dump(obj).should == "\x04\bo:2MarshalSpec::BasicObjectSubWithRespondToFalse\x00"
      end
    end

    it "dumps without marshaling any attached finalizer" do
      obj = Object.new
      finalizer = Object.new
      def finalizer.noop(_)
      end
      NATFIXME 'Implement ObjectSpace', exception: NameError, message: 'uninitialized constant ObjectSpace' do
        ObjectSpace.define_finalizer(obj, finalizer.method(:noop))
        Marshal.load(Marshal.dump(obj)).class.should == Object
      end
    end
  end

  describe "with a Range" do
    # NATFIXME: This breaks with a segfault
    xit "dumps a Range inclusive of end (with indeterminant order)" do
      dump = Marshal.dump(1..2)
      load = Marshal.load(dump)
      load.should == (1..2)
    end

    # NATFIXME: This breaks with a segfault
    xit "dumps a Range exclusive of end (with indeterminant order)" do
      dump = Marshal.dump(1...2)
      load = Marshal.load(dump)
      load.should == (1...2)
    end

    it "raises TypeError with an anonymous Range subclass" do
      -> { Marshal.dump(Class.new(Range).new(1, 2)) }.should raise_error(TypeError, /can't dump anonymous class/)
    end
  end

  describe "with a Time" do
    before :each do
      @internal = Encoding.default_internal
      Encoding.default_internal = Encoding::UTF_8

      NATFIXME 'Implement Time#_dump', exception: NoMethodError, message: "undefined method `_dump' for an instance of Time" do
        @utc = Time.utc(2012, 1, 1)
        @utc_dump = @utc.send(:_dump)

        with_timezone 'AST', 3 do
          @t = Time.local(2012, 1, 1)
          @fract = Time.local(2012, 1, 1, 1, 59, 56.2)
          @t_dump = @t.send(:_dump)
          @fract_dump = @fract.send(:_dump)
        end
      end
    end

    after :each do
      Encoding.default_internal = @internal
    end

    it "dumps the zone and the offset" do
      with_timezone 'AST', 3 do
        dump = Marshal.dump(@t)
        base = "\x04\bIu:\tTime\r#{@t_dump}\a"
        offset = ":\voffseti\x020*"
        zone = ":\tzoneI\"\bAST\x06:\x06EF" # Last is 'F' (US-ASCII)
        NATFIXME 'dumps the zone and the offset', exception: SpecFailedException do
          [ "#{base}#{offset}#{zone}", "#{base}#{zone}#{offset}" ].should include(dump)
        end
      end
    end

    it "dumps the zone, but not the offset if zone is UTC" do
      dump = Marshal.dump(@utc)
      zone = ":\tzoneI\"\bUTC\x06:\x06EF" # Last is 'F' (US-ASCII)
      NATFIXME 'dumps the zone, but not the offset if zone is UTC', exception: SpecFailedException do
        dump.should == "\x04\bIu:\tTime\r#{@utc_dump}\x06#{zone}"
      end
    end

    it "ignores overridden name method" do
      obj = MarshalSpec::TimeWithOverriddenName.new
      NATFIXME 'ignores overridden name method', exception: SpecFailedException do
        Marshal.dump(obj).should include("MarshalSpec::TimeWithOverriddenName")
      end
    end

    it "dumps a Time subclass with multibyte characters in name" do
      NATFIXME 'eval', exception: TypeError, message: 'eval() only works on static strings' do
        source_object = eval("MarshalSpec::MultibyteぁあぃいTime".dup.force_encoding(Encoding::UTF_8))
        Marshal.dump(source_object).should == "\x04\bc+MarshalSpec::Multibyte\xE3\x81\x81\xE3\x81\x82\xE3\x81\x83\xE3\x81\x84Time"
      end
    end

    it "raises TypeError with an anonymous Time subclass" do
      NATFIXME 'raises TypeError with an anonymous Time subclass', exception: SpecFailedException do
        -> { Marshal.dump(Class.new(Time).now) }.should raise_error(TypeError)
      end
    end
  end

  describe "with an Exception" do
    it "dumps an empty Exception" do
      NATFIXME 'dumps an empty Exception', exception: SpecFailedException do
        Marshal.dump(Exception.new).should == "\x04\bo:\x0EException\a:\tmesg0:\abt0"
      end
    end

    it "dumps the message for the exception" do
      NATFIXME 'dumps the message for the exception', exception: SpecFailedException do
        Marshal.dump(Exception.new("foo")).should == "\x04\bo:\x0EException\a:\tmesg\"\bfoo:\abt0"
      end
    end

    it "contains the filename in the backtrace" do
      obj = Exception.new("foo")
      obj.set_backtrace(["foo/bar.rb:10"])
      NATFIXME 'contains the filename in the backtrace', exception: SpecFailedException do
        Marshal.dump(obj).should == "\x04\bo:\x0EException\a:\tmesg\"\bfoo:\abt[\x06\"\x12foo/bar.rb:10"
      end
    end

    it "dumps instance variables if they exist" do
      obj = Exception.new("foo")
      obj.instance_variable_set(:@ivar, 1)
      NATFIXME 'dumps instance variables if they exist', exception: SpecFailedException do
        Marshal.dump(obj).should == "\x04\bo:\x0EException\b:\tmesg\"\bfoo:\abt0:\n@ivari\x06"
      end
    end

    it "dumps the cause for the exception" do
      exc = nil
      begin
        raise StandardError, "the cause"
      rescue StandardError => cause
        begin
          raise RuntimeError, "the consequence"
        rescue RuntimeError => e
          e.cause.should equal(cause)
          exc = e
        end
      end

      reloaded = Marshal.load(Marshal.dump(exc))
      NATFIXME 'dumps the cause for the exception', exception: SpecFailedException do
        reloaded.cause.should be_an_instance_of(StandardError)
        reloaded.cause.message.should == "the cause"
      end
    end

    # NoMethodError uses an exception formatter on TruffleRuby and computes a message lazily
    it "dumps the message for the raised NoMethodError exception" do
      begin
        "".foo
      rescue => e
      end

      NATFIXME 'dumps the message for the raised NoMethodError exception', exception: SpecFailedException do
        Marshal.dump(e).should =~ /undefined method [`']foo' for ("":String|an instance of String)/
      end
    end

    it "raises TypeError if an Object is an instance of an anonymous class" do
      anonymous_class = Class.new(Exception)
      obj = anonymous_class.new

      -> { Marshal.dump(obj) }.should raise_error(TypeError, /can't dump anonymous class/)
    end
  end

  it "dumps subsequent appearances of a symbol as a link" do
    Marshal.dump([:a, :a]).should == "\004\b[\a:\006a;\000"
  end

  it "dumps subsequent appearances of an object as a link" do
    o = Object.new
    NATFIXME 'dumps subsequent appearances of an object as a link', exception: SpecFailedException do
      Marshal.dump([o, o]).should == "\004\b[\ao:\vObject\000@\006"
    end
  end

  MarshalSpec::DATA_19.each do |description, (object, marshal, attributes)|
    it "#{description} returns a binary string" do
      Marshal.dump(object).encoding.should == Encoding::BINARY
    end
  end

  it "raises an ArgumentError when the recursion limit is exceeded" do
    h = {'one' => {'two' => {'three' => 0}}}
    NATFIXME 'Support recursion limit', exception: SpecFailedException, message: "undefined method `ungetbyte' for an instance of Integer" do
      -> { Marshal.dump(h, 3) }.should raise_error(ArgumentError)
      -> { Marshal.dump([h], 4) }.should raise_error(ArgumentError)
      -> { Marshal.dump([], 0) }.should raise_error(ArgumentError)
      -> { Marshal.dump([[[]]], 1) }.should raise_error(ArgumentError)
    end
  end

  it "ignores the recursion limit if the limit is negative" do
    NATFIXME 'Support recursion limit', exception: NoMethodError, message: "undefined method `ungetbyte' for an instance of Integer" do
      Marshal.dump([], -1).should == "\004\b[\000"
      Marshal.dump([[]], -1).should == "\004\b[\006[\000"
      Marshal.dump([[[]]], -1).should == "\004\b[\006[\006[\000"
    end
  end

  describe "when passed an IO" do
    it "writes the serialized data to the IO-Object" do
      NATFIXME 'should not depend on ungetbyte', exception: NoMethodError, message: "undefined method `ungetbyte' for an instance of MockObject" do
        (obj = mock('test')).should_receive(:write).at_least(1)
        Marshal.dump("test", obj)
      end
    end

    it "returns the IO-Object" do
      NATFIXME 'should not depend on ungetbyte', exception: NoMethodError, message: "undefined method `ungetbyte' for an instance of MockObject" do
        (obj = mock('test')).should_receive(:write).at_least(1)
        Marshal.dump("test", obj).should == obj
      end
    end

    it "raises an Error when the IO-Object does not respond to #write" do
      obj = mock('test')
      NATFIXME 'should not depend on ungetbyte', exception: SpecFailedException, message: /undefined method `ungetbyte'/ do
        -> { Marshal.dump("test", obj) }.should raise_error(TypeError)
      end
    end


    it "calls binmode when it's defined" do
      obj = mock('test')
      NATFIXME 'should not depend on ungetbyte', exception: NoMethodError, message: "undefined method `ungetbyte' for an instance of MockObject" do
        obj.should_receive(:write).at_least(1)
        obj.should_receive(:binmode).at_least(1)
        Marshal.dump("test", obj)
      end
    end
  end

  describe "when passed a StringIO" do
    it "should raise an error" do
      require "stringio"
      NATFIXME 'raises a TypeError if marshalling a StringIO instance', exception: SpecFailedException do
        -> { Marshal.dump(StringIO.new) }.should raise_error(TypeError)
      end
    end
  end

  it "raises a TypeError if marshalling a Method instance" do
    NATFIXME 'raises a TypeError if marshalling a Method instance', exception: SpecFailedException do
      -> { Marshal.dump(Marshal.method(:dump)) }.should raise_error(TypeError)
    end
  end

  it "raises a TypeError if marshalling a Proc" do
    NATFIXME 'raises a TypeError if marshalling a Proc', exception: SpecFailedException do
      -> { Marshal.dump(proc {}) }.should raise_error(TypeError)
    end
  end

  it "raises a TypeError if dumping a IO/File instance" do
    NATFIXME 'raises a TypeError if dumping a IO/File instance', exception: SpecFailedException do
      -> { Marshal.dump(STDIN) }.should raise_error(TypeError)
      -> { File.open(__FILE__) { |f| Marshal.dump(f) } }.should raise_error(TypeError)
    end
  end

  it "raises a TypeError if dumping a MatchData instance" do
    NATFIXME 'raises a TypeError if dumping a MatchData instance', exception: SpecFailedException do
      -> { Marshal.dump(/(.)/.match("foo")) }.should raise_error(TypeError)
    end
  end

  it "raises a TypeError if dumping a Mutex instance" do
    m = Mutex.new
    NATFIXME 'raises a TypeError if dumping a Mutex instance', exception: SpecFailedException do
      -> { Marshal.dump(m) }.should raise_error(TypeError)
    end
  end
end
