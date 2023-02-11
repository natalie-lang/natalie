# -*- encoding: utf-8 -*-
require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "String#downcase" do
  it "returns a copy of self with all uppercase letters downcased" do
    "hELLO".downcase.should == "hello"
    "hello".downcase.should == "hello"
  end

  it "returns a String in the same encoding as self" do
    "hELLO".encode("US-ASCII").downcase.encoding.should == Encoding::US_ASCII
  end

  describe "full Unicode case mapping" do
    it "works for all of Unicode with no option" do
      NATFIXME 'Pending unicode casemap support', exception: SpecFailedException do
        "ÄÖÜ".downcase.should == "äöü"
      end
    end

    it "updates string metadata" do
      downcased = "\u{212A}ING".downcase

      NATFIXME 'Pending unicode casemap support', exception: SpecFailedException do
        downcased.should == "king"
        downcased.size.should == 4
        downcased.bytesize.should == 4
        downcased.ascii_only?.should be_true
      end
    end
  end

  describe "ASCII-only case mapping" do
    it "does not downcase non-ASCII characters" do
      "CÅR".downcase(:ascii).should == "cÅr"
    end

    it "works with substrings" do
      "prefix TÉ"[-2..-1].downcase(:ascii).should == "tÉ"
    end
  end

  describe "full Unicode case mapping adapted for Turkic languages" do
    it "downcases characters according to Turkic semantics" do
      NATFIXME 'Pending unicode casemap support', exception: SpecFailedException do
        "İ".downcase(:turkic).should == "i"
      end
    end

    it "allows Lithuanian as an extra option" do
      NATFIXME 'Pending unicode casemap support', exception: SpecFailedException do
        "İ".downcase(:turkic, :lithuanian).should == "i"
      end
    end

    it "does not allow any other additional option" do
      -> { "İ".downcase(:turkic, :ascii) }.should raise_error(ArgumentError)
    end
  end

  describe "full Unicode case mapping adapted for Lithuanian" do
    it "currently works the same as full Unicode case mapping" do
      NATFIXME 'Pending unicode casemap support', exception: SpecFailedException do
        "İS".downcase(:lithuanian).should == "i\u{307}s"
      end
    end

    it "allows Turkic as an extra option (and applies Turkic semantics)" do
      NATFIXME 'Pending unicode casemap support', exception: SpecFailedException do
        "İS".downcase(:lithuanian, :turkic).should == "is"
      end
    end

    it "does not allow any other additional option" do
      -> { "İS".downcase(:lithuanian, :ascii) }.should raise_error(ArgumentError)
    end
  end

  describe "case folding" do
    it "case folds special characters" do
      NATFIXME 'Pending unicode casefold support', exception: SpecFailedException do
        "ß".downcase.should == "ß"
        "ß".downcase(:fold).should == "ss"
      end
    end
  end

  it "does not allow invalid options" do
    -> { "ABC".downcase(:invalid_option) }.should raise_error(ArgumentError)
  end

  ruby_version_is ''...'3.0' do
    it "returns a subclass instance for subclasses" do
      StringSpecs::MyString.new("FOObar").downcase.should be_an_instance_of(StringSpecs::MyString)
    end
  end

  ruby_version_is '3.0' do
    it "returns a String instance for subclasses" do
      StringSpecs::MyString.new("FOObar").downcase.should be_an_instance_of(String)
    end
  end
end

describe "String#downcase!" do
  it "modifies self in place" do
    a = "HeLlO"
    a.downcase!.should equal(a)
    a.should == "hello"
  end

  it "modifies self in place for non-ascii-compatible encodings" do
    a = "HeLlO".encode("utf-16le")
    a.downcase!
    a.should == "hello".encode("utf-16le")
  end

  describe "full Unicode case mapping" do
    it "modifies self in place for all of Unicode with no option" do
      a = "ÄÖÜ"
      a.downcase!
      NATFIXME 'Pending unicode casemap support', exception: SpecFailedException do
        a.should == "äöü"
      end
    end

    it "updates string metadata" do
      downcased = "\u{212A}ING"
      downcased.downcase!

      NATFIXME 'Pending unicode casemap support', exception: SpecFailedException do
        downcased.should == "king"
        downcased.size.should == 4
        downcased.bytesize.should == 4
        downcased.ascii_only?.should be_true
      end
    end
  end

  describe "ASCII-only case mapping" do
    it "does not downcase non-ASCII characters" do
      a = "CÅR"
      a.downcase!(:ascii)
      a.should == "cÅr"
    end

    it "works for non-ascii-compatible encodings" do
      a = "ABC".encode("utf-16le")
      a.downcase!(:ascii)
      a.should == "abc".encode("utf-16le")
    end
  end

  describe "full Unicode case mapping adapted for Turkic languages" do
    it "downcases characters according to Turkic semantics" do
      a = "İ"
      a.downcase!(:turkic)
      NATFIXME 'Pending unicode casemap support', exception: SpecFailedException do
        a.should == "i"
      end
    end

    it "allows Lithuanian as an extra option" do
      a = "İ"
      a.downcase!(:turkic, :lithuanian)
      NATFIXME 'Pending unicode casemap support', exception: SpecFailedException do
        a.should == "i"
      end
    end

    it "does not allow any other additional option" do
      -> { a = "İ"; a.downcase!(:turkic, :ascii) }.should raise_error(ArgumentError)
    end
  end

  describe "full Unicode case mapping adapted for Lithuanian" do
    it "currently works the same as full Unicode case mapping" do
      a = "İS"
      a.downcase!(:lithuanian)
      NATFIXME 'Pending unicode casemap support', exception: SpecFailedException do
        a.should == "i\u{307}s"
      end
    end

    it "allows Turkic as an extra option (and applies Turkic semantics)" do
      a = "İS"
      a.downcase!(:lithuanian, :turkic)
      NATFIXME 'Pending unicode casemap support', exception: SpecFailedException do
        a.should == "is"
      end
    end

    it "does not allow any other additional option" do
      -> { a = "İS"; a.downcase!(:lithuanian, :ascii) }.should raise_error(ArgumentError)
    end
  end

  describe "case folding" do
    it "case folds special characters" do
      a = "ß"
      a.downcase!
      a.should == "ß"

      a.downcase!(:fold)
      NATFIXME 'Pending unicode casefold support', exception: SpecFailedException do
        a.should == "ss"
      end
    end
  end

  it "does not allow invalid options" do
    -> { a = "ABC"; a.downcase!(:invalid_option) }.should raise_error(ArgumentError)
  end

  it "returns nil if no modifications were made" do
    a = "hello"
    a.downcase!.should == nil
    a.should == "hello"
  end

  it "raises a FrozenError when self is frozen" do
    -> { "HeLlo".freeze.downcase! }.should raise_error(FrozenError)
    -> { "hello".freeze.downcase! }.should raise_error(FrozenError)
  end

  it "sets the result String encoding to the source String encoding" do
    "ABC".downcase.encoding.should equal(Encoding::UTF_8)
  end
end
