# -*- encoding: binary -*-
require_relative '../../spec_helper'
require_relative '../fixtures/classes'

describe "Regexps with encoding modifiers" do
  it "supports /e (EUC encoding)" do
    match = /./e.match("\303\251".dup.force_encoding(Encoding::EUC_JP))
    match.to_a.should == ["\303\251".dup.force_encoding(Encoding::EUC_JP)]
  end

  it "supports /e (EUC encoding) with interpolation" do
    match = /#{/./}/e.match("\303\251".dup.force_encoding(Encoding::EUC_JP))
    match.to_a.should == ["\303\251".dup.force_encoding(Encoding::EUC_JP)]
  end

  it "supports /e (EUC encoding) with interpolation /o" do
    match = /#{/./}/e.match("\303\251".dup.force_encoding(Encoding::EUC_JP))
    match.to_a.should == ["\303\251".dup.force_encoding(Encoding::EUC_JP)]
  end

  it 'uses EUC-JP as /e encoding' do
    NATFIXME 'it uses EUC-JP as /e encoding', exception: SpecFailedException do
      /./e.encoding.should == Encoding::EUC_JP
    end
  end

  it 'preserves EUC-JP as /e encoding through interpolation' do
    NATFIXME 'it preserves EUC-JP as /e encoding through interpolation', exception: SpecFailedException do
      /#{/./}/e.encoding.should == Encoding::EUC_JP
    end
  end

  it "supports /n (No encoding)" do
    NATFIXME 'it supports /n (No encoding)', exception: SpecFailedException do
      /./n.match("\303\251").to_a.should == ["\303"]
    end
  end

  it "supports /n (No encoding) with interpolation" do
    NATFIXME 'it supports /n (No encoding) with interpolation', exception: SpecFailedException do
      /#{/./}/n.match("\303\251").to_a.should == ["\303"]
    end
  end

  it "supports /n (No encoding) with interpolation /o" do
    NATFIXME 'it supports /n (No encoding) with interpolation /o', exception: SpecFailedException do
      /#{/./}/n.match("\303\251").to_a.should == ["\303"]
    end
  end

  it "warns when using /n with a match string with non-ASCII characters and an encoding other than ASCII-8BIT" do
    NATFIXME 'it warns when using /n with a match string with non-ASCII characters and an encoding other than ASCII-8BIT', exception: SpecFailedException do
      -> { /./n.match("\303\251".dup.force_encoding('utf-8')) }.should complain(%r{historical binary regexp match /.../n against UTF-8 string})
    end
  end

  it 'uses US-ASCII as /n encoding if all chars are 7-bit' do
    /./n.encoding.should == Encoding::US_ASCII
  end

  it 'uses BINARY when is not initialized' do
    NATFIXME 'it uses BINARY when is not initialized', exception: SpecFailedException do
      Regexp.allocate.encoding.should == Encoding::BINARY
    end
  end

  it 'uses BINARY as /n encoding if not all chars are 7-bit' do
    NATFIXME 'it uses BINARY as /n encoding if not all chars are 7-bit', exception: SpecFailedException do
      /\xFF/n.encoding.should == Encoding::BINARY
    end
  end

  it 'preserves US-ASCII as /n encoding through interpolation if all chars are 7-bit' do
    /.#{/./}/n.encoding.should == Encoding::US_ASCII
  end

  it 'preserves BINARY as /n encoding through interpolation if all chars are 7-bit' do
    NATFIXME 'it preserves BINARY as /n encoding through interpolation if all chars are 7-bit', exception: SpecFailedException do
      /\xFF#{/./}/n.encoding.should == Encoding::BINARY
    end
  end

  it "supports /s (Windows_31J encoding)" do
    NATFIXME 'Implement Windows-31J encoding', exception: NameError, message: 'uninitialized constant Encoding::Windows_31J' do
      match = /./s.match("\303\251".dup.force_encoding(Encoding::Windows_31J))
      match.to_a.should == ["\303".dup.force_encoding(Encoding::Windows_31J)]
    end
  end

  it "supports /s (Windows_31J encoding) with interpolation" do
    NATFIXME 'Implement Windows-31J encoding', exception: NameError, message: 'uninitialized constant Encoding::Windows_31J' do
      match = /#{/./}/s.match("\303\251".dup.force_encoding(Encoding::Windows_31J))
      match.to_a.should == ["\303".dup.force_encoding(Encoding::Windows_31J)]
    end
  end

  it "supports /s (Windows_31J encoding) with interpolation and /o" do
    NATFIXME 'Implement Windows-31J encoding', exception: NameError, message: 'uninitialized constant Encoding::Windows_31J' do
      match = /#{/./}/s.match("\303\251".dup.force_encoding(Encoding::Windows_31J))
      match.to_a.should == ["\303".dup.force_encoding(Encoding::Windows_31J)]
    end
  end

  it 'uses Windows-31J as /s encoding' do
    NATFIXME 'Implement Windows-31J encoding', exception: NameError, message: 'uninitialized constant Encoding::Windows_31J' do
      /./s.encoding.should == Encoding::Windows_31J
    end
  end

  it 'preserves Windows-31J as /s encoding through interpolation' do
    NATFIXME 'Implement Windows-31J encoding', exception: NameError, message: 'uninitialized constant Encoding::Windows_31J' do
      /#{/./}/s.encoding.should == Encoding::Windows_31J
    end
  end

  it "supports /u (UTF8 encoding)" do
    /./u.match("\303\251".dup.force_encoding('utf-8')).to_a.should == ["\u{e9}"]
  end

  it "supports /u (UTF8 encoding) with interpolation" do
    /#{/./}/u.match("\303\251".dup.force_encoding('utf-8')).to_a.should == ["\u{e9}"]
  end

  it "supports /u (UTF8 encoding) with interpolation and /o" do
    /#{/./}/u.match("\303\251".dup.force_encoding('utf-8')).to_a.should == ["\u{e9}"]
  end

  it 'uses UTF-8 as /u encoding' do
    NATFIXME 'it uses UTF-8 as /u encoding', exception: SpecFailedException do
      /./u.encoding.should == Encoding::UTF_8
    end
  end

  it 'preserves UTF-8 as /u encoding through interpolation' do
    NATFIXME 'it preserves UTF-8 as /u encoding through interpolation', exception: SpecFailedException do
      /#{/./}/u.encoding.should == Encoding::UTF_8
    end
  end

  it "selects last of multiple encoding specifiers" do
    /foo/ensuensuens.should == /foo/s
  end

  it "raises Encoding::CompatibilityError when trying match against different encodings" do
    NATFIXME 'it raises Encoding::CompatibilityError when trying match against different encodings', exception: SpecFailedException do
      -> { /\A[[:space:]]*\z/.match(" ".encode("UTF-16LE")) }.should raise_error(Encoding::CompatibilityError)
    end
  end

  it "raises Encoding::CompatibilityError when trying match? against different encodings" do
    NATFIXME 'it raises Encoding::CompatibilityError when trying match? against different encodings', exception: SpecFailedException do
      -> { /\A[[:space:]]*\z/.match?(" ".encode("UTF-16LE")) }.should raise_error(Encoding::CompatibilityError)
    end
  end

  it "raises Encoding::CompatibilityError when trying =~ against different encodings" do
    NATFIXME 'it raises Encoding::CompatibilityError when trying =~ against different encodings', exception: SpecFailedException do
      -> { /\A[[:space:]]*\z/ =~ " ".encode("UTF-16LE") }.should raise_error(Encoding::CompatibilityError)
    end
  end

  it "raises Encoding::CompatibilityError when the regexp has a fixed, non-ASCII-compatible encoding" do
    NATFIXME 'it raises Encoding::CompatibilityError when the regexp has a fixed, non-ASCII-compatible encoding', exception: SpecFailedException do
      -> { Regexp.new("".dup.force_encoding("UTF-16LE"), Regexp::FIXEDENCODING) =~ " ".encode("UTF-8") }.should raise_error(Encoding::CompatibilityError)
    end
  end

  it "raises Encoding::CompatibilityError when the regexp has a fixed encoding and the match string has non-ASCII characters" do
    NATFIXME 'it raises Encoding::CompatibilityError when the regexp has a fixed encoding and the match string has non-ASCII characters', exception: SpecFailedException do
      -> { Regexp.new("".dup.force_encoding("US-ASCII"), Regexp::FIXEDENCODING) =~ "\303\251".dup.force_encoding('UTF-8') }.should raise_error(Encoding::CompatibilityError)
    end
  end

  it "raises ArgumentError when trying to match a broken String" do
    s = "\x80".dup.force_encoding('UTF-8')
    NATFIXME 'it raises ArgumentError when trying to match a broken String', exception: SpecFailedException do
      -> { s =~ /./ }.should raise_error(ArgumentError, "invalid byte sequence in UTF-8")
    end
  end

  it "computes the Regexp Encoding for each interpolated Regexp instance" do
    make_regexp = -> str { /#{str}/ }

    r = make_regexp.call("été".dup.force_encoding(Encoding::UTF_8))
    NATFIXME 'Implement Regexp#fixed_encoding?', exception: NoMethodError, message: "undefined method `fixed_encoding?' for an instance of Regexp" do
      r.should.fixed_encoding?
    end
    NATFIXME 'it computes the Regexp Encoding for each interpolated Regexp instance', exception: SpecFailedException do
      r.encoding.should == Encoding::UTF_8
    end

    r = make_regexp.call("abc".dup.force_encoding(Encoding::UTF_8))
    NATFIXME 'Implement Regexp#fixed_encoding?', exception: NoMethodError, message: "undefined method `fixed_encoding?' for an instance of Regexp" do
      r.should_not.fixed_encoding?
    end
    r.encoding.should == Encoding::US_ASCII
  end
end
