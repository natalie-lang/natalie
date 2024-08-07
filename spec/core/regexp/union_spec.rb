# encoding: utf-8

require_relative '../../spec_helper'

describe "Regexp.union" do
  it "returns /(?!)/ when passed no arguments" do
    Regexp.union.should == /(?!)/
  end

  it "returns a regular expression that will match passed arguments" do
    Regexp.union("penzance").should == /penzance/
    Regexp.union("skiing", "sledding").should == /skiing|sledding/
    not_supported_on :opal do
      Regexp.union(/dogs/, /cats/i).should == /(?-mix:dogs)|(?i-mx:cats)/
    end
  end

  it "quotes any string arguments" do
    Regexp.union("n", ".").should == /n|\./
  end

  it "returns a Regexp with the encoding of an ASCII-incompatible String argument" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      Regexp.union("a".encode("UTF-16LE")).encoding.should == Encoding::UTF_16LE
    end
  end

  it "returns a Regexp with the encoding of a String containing non-ASCII-compatible characters" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      Regexp.union("\u00A9".encode("ISO-8859-1")).encoding.should == Encoding::ISO_8859_1
    end
  end

  it "returns a Regexp with US-ASCII encoding if all arguments are ASCII-only" do
    NATFIXME 'Implement SJIS encoding', exception: Encoding::ConverterNotFoundError, message: 'code converter not found (UTF-8 to SJIS)' do
      Regexp.union("a".encode("UTF-8"), "b".encode("SJIS")).encoding.should == Encoding::US_ASCII
    end
  end

  it "returns a Regexp with the encoding of multiple non-conflicting ASCII-incompatible String arguments" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      Regexp.union("a".encode("UTF-16LE"), "b".encode("UTF-16LE")).encoding.should == Encoding::UTF_16LE
    end
  end

  it "returns a Regexp with the encoding of multiple non-conflicting Strings containing non-ASCII-compatible characters" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      Regexp.union("\u00A9".encode("ISO-8859-1"), "\u00B0".encode("ISO-8859-1")).encoding.should == Encoding::ISO_8859_1
    end
  end

  it "returns a Regexp with the encoding of a String containing non-ASCII-compatible characters and another ASCII-only String" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      Regexp.union("\u00A9".encode("ISO-8859-1"), "a".encode("UTF-8")).encoding.should == Encoding::ISO_8859_1
    end
  end

  it "returns ASCII-8BIT if the regexp encodings are ASCII-8BIT and at least one has non-ASCII characters" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      us_ascii_implicit, us_ascii_explicit, binary = /abc/, /[\x00-\x7f]/n, /[\x80-\xBF]/n
      us_ascii_implicit.encoding.should == Encoding::US_ASCII
      us_ascii_explicit.encoding.should == Encoding::US_ASCII
      binary.encoding.should == Encoding::BINARY

      Regexp.union(us_ascii_implicit, us_ascii_explicit, binary).encoding.should == Encoding::BINARY
      Regexp.union(us_ascii_implicit, binary, us_ascii_explicit).encoding.should == Encoding::BINARY
      Regexp.union(us_ascii_explicit, us_ascii_implicit, binary).encoding.should == Encoding::BINARY
      Regexp.union(us_ascii_explicit, binary, us_ascii_implicit).encoding.should == Encoding::BINARY
      Regexp.union(binary, us_ascii_implicit, us_ascii_explicit).encoding.should == Encoding::BINARY
      Regexp.union(binary, us_ascii_explicit, us_ascii_implicit).encoding.should == Encoding::BINARY
    end
  end

  it "return US-ASCII if all patterns are ASCII-only" do
    Regexp.union(/abc/e, /def/e).encoding.should == Encoding::US_ASCII
    Regexp.union(/abc/n, /def/n).encoding.should == Encoding::US_ASCII
    Regexp.union(/abc/s, /def/s).encoding.should == Encoding::US_ASCII
    Regexp.union(/abc/u, /def/u).encoding.should == Encoding::US_ASCII
  end

  it "returns a Regexp with UTF-8 if one part is UTF-8" do
    Regexp.union(/probl[éeè]me/i, /help/i).encoding.should == Encoding::UTF_8
  end

  it "returns a Regexp if an array of string with special characters is passed" do
    Regexp.union(["+","-"]).should == /\+|\-/
  end

  it "raises ArgumentError if the arguments include conflicting ASCII-incompatible Strings" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union("a".encode("UTF-16LE"), "b".encode("UTF-16BE"))
      }.should raise_error(ArgumentError, 'incompatible encodings: UTF-16LE and UTF-16BE')
    end
  end

  it "raises ArgumentError if the arguments include conflicting ASCII-incompatible Regexps" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union(Regexp.new("a".encode("UTF-16LE")),
                     Regexp.new("b".encode("UTF-16BE")))
      }.should raise_error(ArgumentError, 'incompatible encodings: UTF-16LE and UTF-16BE')
    end
  end

  it "raises ArgumentError if the arguments include conflicting fixed encoding Regexps" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union(Regexp.new("a".encode("UTF-8"),    Regexp::FIXEDENCODING),
                     Regexp.new("b".encode("US-ASCII"), Regexp::FIXEDENCODING))
      }.should raise_error(ArgumentError, 'incompatible encodings: UTF-8 and US-ASCII')
    end
  end

  it "raises ArgumentError if the arguments include a fixed encoding Regexp and a String containing non-ASCII-compatible characters in a different encoding" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union(Regexp.new("a".encode("UTF-8"), Regexp::FIXEDENCODING),
                     "\u00A9".encode("ISO-8859-1"))
      }.should raise_error(ArgumentError, 'incompatible encodings: UTF-8 and ISO-8859-1')
    end
  end

  it "raises ArgumentError if the arguments include a String containing non-ASCII-compatible characters and a fixed encoding Regexp in a different encoding" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union("\u00A9".encode("ISO-8859-1"),
                     Regexp.new("a".encode("UTF-8"), Regexp::FIXEDENCODING))
      }.should raise_error(ArgumentError, 'incompatible encodings: ISO-8859-1 and UTF-8')
    end
  end

  it "raises ArgumentError if the arguments include an ASCII-incompatible String and an ASCII-only String" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union("a".encode("UTF-16LE"), "b".encode("UTF-8"))
      }.should raise_error(ArgumentError, /ASCII incompatible encoding: UTF-16LE|incompatible encodings: UTF-16LE and US-ASCII/)
    end
  end

  it "raises ArgumentError if the arguments include an ASCII-incompatible Regexp and an ASCII-only String" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union(Regexp.new("a".encode("UTF-16LE")), "b".encode("UTF-8"))
      }.should raise_error(ArgumentError, /ASCII incompatible encoding: UTF-16LE|incompatible encodings: UTF-16LE and US-ASCII/)
    end
  end

  it "raises ArgumentError if the arguments include an ASCII-incompatible String and an ASCII-only Regexp" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union("a".encode("UTF-16LE"), Regexp.new("b".encode("UTF-8")))
      }.should raise_error(ArgumentError, /ASCII incompatible encoding: UTF-16LE|incompatible encodings: UTF-16LE and US-ASCII/)
    end
  end

  it "raises ArgumentError if the arguments include an ASCII-incompatible Regexp and an ASCII-only Regexp" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union(Regexp.new("a".encode("UTF-16LE")), Regexp.new("b".encode("UTF-8")))
      }.should raise_error(ArgumentError, /ASCII incompatible encoding: UTF-16LE|incompatible encodings: UTF-16LE and US-ASCII/)
    end
  end

  it "raises ArgumentError if the arguments include an ASCII-incompatible String and a String containing non-ASCII-compatible characters in a different encoding" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union("a".encode("UTF-16LE"), "\u00A9".encode("ISO-8859-1"))
      }.should raise_error(ArgumentError, 'incompatible encodings: UTF-16LE and ISO-8859-1')
    end
  end

  it "raises ArgumentError if the arguments include an ASCII-incompatible Regexp and a String containing non-ASCII-compatible characters in a different encoding" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union(Regexp.new("a".encode("UTF-16LE")), "\u00A9".encode("ISO-8859-1"))
      }.should raise_error(ArgumentError, 'incompatible encodings: UTF-16LE and ISO-8859-1')
    end
  end

  it "raises ArgumentError if the arguments include an ASCII-incompatible String and a Regexp containing non-ASCII-compatible characters in a different encoding" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union("a".encode("UTF-16LE"), Regexp.new("\u00A9".encode("ISO-8859-1")))
      }.should raise_error(ArgumentError, 'incompatible encodings: UTF-16LE and ISO-8859-1')
    end
  end

  it "raises ArgumentError if the arguments include an ASCII-incompatible Regexp and a Regexp containing non-ASCII-compatible characters in a different encoding" do
    NATFIXME 'Encodings', exception: SpecFailedException do
      -> {
        Regexp.union(Regexp.new("a".encode("UTF-16LE")), Regexp.new("\u00A9".encode("ISO-8859-1")))
      }.should raise_error(ArgumentError, 'incompatible encodings: UTF-16LE and ISO-8859-1')
    end
  end

  it "uses to_str to convert arguments (if not Regexp)" do
    obj = mock('pattern')
    obj.should_receive(:to_str).and_return('foo')
    Regexp.union(obj, "bar").should == /foo|bar/
  end

  it "uses to_regexp to convert argument" do
    obj = mock('pattern')
    obj.should_receive(:to_regexp).and_return(/foo/)
    Regexp.union(obj).should == /foo/
  end

  it "accepts a Symbol as argument" do
    Regexp.union(:foo).should == /foo/
  end

  it "accepts a single array of patterns as arguments" do
    Regexp.union(["skiing", "sledding"]).should == /skiing|sledding/
    not_supported_on :opal do
      Regexp.union([/dogs/, /cats/i]).should == /(?-mix:dogs)|(?i-mx:cats)/
    end
    -> {
      Regexp.union(["skiing", "sledding"], [/dogs/, /cats/i])
    }.should raise_error(TypeError, 'no implicit conversion of Array into String')
  end
end
