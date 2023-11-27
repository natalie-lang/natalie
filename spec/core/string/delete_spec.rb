# -*- encoding: utf-8 -*-
require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "String#delete" do
  it "returns a new string with the chars from the intersection of sets removed" do
    s = "hello"
    NATFIXME 'returns a new string with the chars from the intersection of sets removed', exception: SpecFailedException do
      s.delete("lo").should == "he"
      s.should == "hello"

      "hello".delete("l", "lo").should == "heo"

      "hell yeah".delete("").should == "hell yeah"
    end
  end

  it "raises an ArgumentError when given no arguments" do
    -> { "hell yeah".delete }.should raise_error(ArgumentError)
  end

  it "negates sets starting with ^" do
    NATFIXME 'negates sets starting with ^', exception: SpecFailedException do
      "hello".delete("aeiou", "^e").should == "hell"
      "hello".delete("^leh").should == "hell"
      "hello".delete("^o").should == "o"
      "hello".delete("^").should == "hello"
      "^_^".delete("^^").should == "^^"
      "oa^_^o".delete("a^").should == "o_o"
    end
  end

  it "deletes all chars in a sequence" do
    NATFIXME 'deletes all chars in a sequence', exception: SpecFailedException do
      "hello".delete("ej-m").should == "ho"
      "hello".delete("e-h").should == "llo"
      "hel-lo".delete("e-").should == "hllo"
      "hel-lo".delete("-h").should == "ello"
      "hel-lo".delete("---").should == "hello"
      "hel-012".delete("--2").should == "hel"
      "hel-()".delete("(--").should == "hel"
      "hello".delete("^e-h").should == "he"
      "hello^".delete("^^-^").should == "^"
      "hel--lo".delete("^---").should == "--"

      "abcdefgh".delete("a-ce-fh").should == "dg"
      "abcdefgh".delete("he-fa-c").should == "dg"
      "abcdefgh".delete("e-fha-c").should == "dg"

      "abcde".delete("ac-e").should == "b"
      "abcde".delete("^ac-e").should == "acde"

      "ABCabc[]".delete("A-a").should == "bc"
    end
  end

  it "deletes multibyte characters" do
    NATFIXME 'deletes multibyte characters', exception: SpecFailedException do
      "四月".delete("月").should == "四"
      '哥哥我倒'.delete('哥').should == "我倒"
    end
  end

  it "respects backslash for escaping a -" do
    NATFIXME 'respects backslash for escaping a -', exception: ArgumentError do
      'Non-Authoritative Information'.delete(' \-\'').should ==
        'NonAuthoritativeInformation'
    end
  end

  it "raises if the given ranges are invalid" do
    NATFIXME 'raises if the given ranges are invalid', exception: SpecFailedException do
      not_supported_on :opal do
        xFF = [0xFF].pack('C')
        range = "\x00 - #{xFF}".force_encoding('utf-8')
        -> { "hello".delete(range).should == "" }.should raise_error(ArgumentError)
      end
      -> { "hello".delete("h-e") }.should raise_error(ArgumentError)
      -> { "hello".delete("^h-e") }.should raise_error(ArgumentError)
    end
  end

  it "tries to convert each set arg to a string using to_str" do
    NATFIXME "tries to convert each set arg to a string using to_str", exception: SpecFailedException do
      other_string = mock('lo')
      other_string.should_receive(:to_str).and_return("lo")

      other_string2 = mock('o')
      other_string2.should_receive(:to_str).and_return("o")

      "hello world".delete(other_string, other_string2).should == "hell wrld"
    end
  end

  it "raises a TypeError when one set arg can't be converted to a string" do
    -> { "hello world".delete(100)       }.should raise_error(TypeError)
    -> { "hello world".delete([])        }.should raise_error(TypeError)
    -> { "hello world".delete(mock('x')) }.should raise_error(TypeError)
  end

  it "returns String instances when called on a subclass" do
    StringSpecs::MyString.new("oh no!!!").delete("!").should be_an_instance_of(String)
  end

  it "returns a String in the same encoding as self" do
    "hello".encode("US-ASCII").delete("lo").encoding.should == Encoding::US_ASCII
  end
end

describe "String#delete!" do
  it "modifies self in place and returns self" do
    a = "hello"
    NATFIXME 'modifies self in place and returns self', exception: SpecFailedException do
      a.delete!("aeiou", "^e").should equal(a)
      a.should == "hell"
    end
  end

  it "returns nil if no modifications were made" do
    a = "hello"
    a.delete!("z").should == nil
    a.should == "hello"
  end

  it "raises a FrozenError when self is frozen" do
    a = "hello"
    a.freeze

    -> { a.delete!("")            }.should raise_error(FrozenError)
    -> { a.delete!("aeiou", "^e") }.should raise_error(FrozenError)
  end
end
