# -*- encoding: utf-8 -*-
# frozen_string_literal: false
require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe :string_gsub_named_capture, shared: true do
  it "replaces \\k named backreferences with the regexp's corresponding capture" do
    str = "hello"

    NATFIXME 'Unknown backslash reference: \k', exception: SpecFailedException do
      str.gsub(/(?<foo>[aeiou])/, '<\k<foo>>').should == "h<e>ll<o>"
      str.gsub(/(?<foo>.)/, '\k<foo>\k<foo>').should == "hheelllloo"
    end
  end
end

describe "String#gsub with pattern and replacement" do
  it "inserts the replacement around every character when the pattern collapses" do
    "hello".gsub(//, ".").should == ".h.e.l.l.o."
  end

  it "respects unicode when the pattern collapses" do
    str = "こにちわ"
    reg = %r!!

    str.gsub(reg, ".").should == ".こ.に.ち.わ."
  end

  it "doesn't freak out when replacing ^" do
    "Text\n".gsub(/^/, ' ').should == " Text\n"
    "Text\nFoo".gsub(/^/, ' ').should == " Text\n Foo"
  end

  it "returns a copy of self with all occurrences of pattern replaced with replacement" do
    "hello".gsub(/[aeiou]/, '*').should == "h*ll*"

    str = "hello homely world. hah!"
    str.gsub(/\Ah\S+\s*/, "huh? ").should == "huh? homely world. hah!"

    str = "¿por qué?"
    str.gsub(/([a-z\d]*)/, "*").should == "*¿** **é*?*"
  end

  it "ignores a block if supplied" do
    "food".gsub(/f/, "g") { "w" }.should == "good"
  end

  it "supports \\G which matches at the beginning of the remaining (non-matched) string" do
    str = "hello homely world. hah!"
    str.gsub(/\Gh\S+\s*/, "huh? ").should == "huh? huh? world. hah!"
  end

  it "supports /i for ignoring case" do
    str = "Hello. How happy are you?"
    str.gsub(/h/i, "j").should == "jello. jow jappy are you?"
    str.gsub(/H/i, "j").should == "jello. jow jappy are you?"
  end

  it "doesn't interpret regexp metacharacters if pattern is a string" do
    "12345".gsub('\d', 'a').should == "12345"
    '\d'.gsub('\d', 'a').should == "a"
  end

  it "replaces \\1 sequences with the regexp's corresponding capture" do
    str = "hello"

    str.gsub(/([aeiou])/, '<\1>').should == "h<e>ll<o>"
    str.gsub(/(.)/, '\1\1').should == "hheelllloo"

    str.gsub(/.(.?)/, '<\0>(\1)').should == "<he>(e)<ll>(l)<o>()"

    str.gsub(/.(.)+/, '\1').should == "o"

    str = "ABCDEFGHIJKLabcdefghijkl"
    re = /#{"(.)" * 12}/
    str.gsub(re, '\1').should == "Aa"
    str.gsub(re, '\9').should == "Ii"
    # Only the first 9 captures can be accessed in MRI
    str.gsub(re, '\10').should == "A0a0"
  end

  it "treats \\1 sequences without corresponding captures as empty strings" do
    str = "hello!"

    str.gsub("", '<\1>').should == "<>h<>e<>l<>l<>o<>!<>"
    str.gsub("h", '<\1>').should == "<>ello!"

    str.gsub(//, '<\1>').should == "<>h<>e<>l<>l<>o<>!<>"
    str.gsub(/./, '\1\2\3').should == ""
    str.gsub(/.(.{20})?/, '\1').should == ""
  end

  it "replaces \\& and \\0 with the complete match" do
    str = "hello!"

    str.gsub("", '<\0>').should == "<>h<>e<>l<>l<>o<>!<>"
    str.gsub("", '<\&>').should == "<>h<>e<>l<>l<>o<>!<>"
    str.gsub("he", '<\0>').should == "<he>llo!"
    str.gsub("he", '<\&>').should == "<he>llo!"
    str.gsub("l", '<\0>').should == "he<l><l>o!"
    str.gsub("l", '<\&>').should == "he<l><l>o!"

    str.gsub(//, '<\0>').should == "<>h<>e<>l<>l<>o<>!<>"
    str.gsub(//, '<\&>').should == "<>h<>e<>l<>l<>o<>!<>"
    str.gsub(/../, '<\0>').should == "<he><ll><o!>"
    str.gsub(/../, '<\&>').should == "<he><ll><o!>"
    str.gsub(/(.)./, '<\0>').should == "<he><ll><o!>"
  end

  it "replaces \\` with everything before the current match" do
    str = "hello!"

    str.gsub("", '<\`>').should == "<>h<h>e<he>l<hel>l<hell>o<hello>!<hello!>"
    str.gsub("h", '<\`>').should == "<>ello!"
    str.gsub("l", '<\`>').should == "he<he><hel>o!"
    str.gsub("!", '<\`>').should == "hello<hello>"

    str.gsub(//, '<\`>').should == "<>h<h>e<he>l<hel>l<hell>o<hello>!<hello!>"
    str.gsub(/../, '<\`>').should == "<><he><hell>"
  end

  it "replaces \\' with everything after the current match" do
    str = "hello!"

    str.gsub("", '<\\\'>').should == "<hello!>h<ello!>e<llo!>l<lo!>l<o!>o<!>!<>"
    str.gsub("h", '<\\\'>').should == "<ello!>ello!"
    str.gsub("ll", '<\\\'>').should == "he<o!>o!"
    str.gsub("!", '<\\\'>').should == "hello<>"

    str.gsub(//, '<\\\'>').should == "<hello!>h<ello!>e<llo!>l<lo!>l<o!>o<!>!<>"
    str.gsub(/../, '<\\\'>').should == "<llo!><o!><>"
  end

  it "replaces \\+ with the last paren that actually matched" do
    str = "hello!"

    str.gsub(/(.)(.)/, '\+').should == "el!"
    str.gsub(/(.)(.)+/, '\+').should == "!"
    str.gsub(/(.)()/, '\+').should == ""
    str.gsub(/(.)(.{20})?/, '<\+>').should == "<h><e><l><l><o><!>"

    str = "ABCDEFGHIJKLabcdefghijkl"
    re = /#{"(.)" * 12}/
    str.gsub(re, '\+').should == "Ll"
  end

  it "treats \\+ as an empty string if there was no captures" do
    "hello!".gsub(/./, '\+').should == ""
  end

  it "maps \\\\ in replacement to \\" do
    "hello".gsub(/./, '\\\\').should == '\\' * 5
  end

  it "leaves unknown \\x escapes in replacement untouched" do
    "hello".gsub(/./, '\\x').should == '\\x' * 5
    "hello".gsub(/./, '\\y').should == '\\y' * 5
  end

  it "leaves \\ at the end of replacement untouched" do
    "hello".gsub(/./, 'hah\\').should == 'hah\\' * 5
  end

  it_behaves_like :string_gsub_named_capture, :gsub

  it "handles pattern collapse" do
    str = "こにちわ"
    reg = %r!!
    str.gsub(reg, ".").should == ".こ.に.ち.わ."
  end

  it "tries to convert pattern to a string using to_str" do
    pattern = mock('.')
    def pattern.to_str() "." end

    "hello.".gsub(pattern, "!").should == "hello!"
  end

  it "raises a TypeError when pattern can't be converted to a string" do
    -> { "hello".gsub([], "x")            }.should raise_error(TypeError)
    -> { "hello".gsub(Object.new, "x")    }.should raise_error(TypeError)
    -> { "hello".gsub(nil, "x")           }.should raise_error(TypeError)
  end

  it "tries to convert replacement to a string using to_str" do
    replacement = mock('hello_replacement')
    def replacement.to_str() "hello_replacement" end

    "hello".gsub(/hello/, replacement).should == "hello_replacement"
  end

  it "raises a TypeError when replacement can't be converted to a string" do
    -> { "hello".gsub(/[aeiou]/, [])            }.should raise_error(TypeError)
    -> { "hello".gsub(/[aeiou]/, Object.new)    }.should raise_error(TypeError)
    -> { "hello".gsub(/[aeiou]/, nil)           }.should raise_error(TypeError)
  end

  it "returns String instances when called on a subclass" do
    StringSpecs::MyString.new("").gsub(//, "").should be_an_instance_of(String)
    StringSpecs::MyString.new("").gsub(/foo/, "").should be_an_instance_of(String)
    StringSpecs::MyString.new("foo").gsub(/foo/, "").should be_an_instance_of(String)
    StringSpecs::MyString.new("foo").gsub("foo", "").should be_an_instance_of(String)
  end

  it "sets $~ to MatchData of last match and nil when there's none" do
    'hello.'.gsub('hello', 'x')
    NATFIXME 'Implement $~', exception: NoMethodError, message: /undefined method [`']\[\]' for nil/ do
      $~[0].should == 'hello'

      'hello.'.gsub('not', 'x')
      $~.should == nil

      'hello.'.gsub(/.(.)/, 'x')
      $~[0].should == 'o.'

      'hello.'.gsub(/not/, 'x')
      $~.should == nil
    end
  end

  it "handles a pattern in a superset encoding" do
    result = 'abc'.force_encoding(Encoding::US_ASCII).gsub('é', 'è')
    result.should == 'abc'
    result.encoding.should == Encoding::US_ASCII
  end

  it "handles a pattern in a subset encoding" do
    result = 'été'.gsub('t'.force_encoding(Encoding::US_ASCII), 'u')
    result.should == 'éué'
    result.encoding.should == Encoding::UTF_8
  end
end

describe "String#gsub with pattern and Hash" do
  it "returns a copy of self with all occurrences of pattern replaced with the value of the corresponding hash key" do
    "hello".gsub(/./, 'l' => 'L').should == "LL"
    "hello!".gsub(/(.)(.)/, 'he' => 'she ', 'll' => 'said').should == 'she said'
    "hello".gsub('l', 'l' => 'el').should == 'heelelo'
  end

  it "ignores keys that don't correspond to matches" do
    "hello".gsub(/./, 'z' => 'L', 'h' => 'b', 'o' => 'ow').should == "bow"
  end

  it "returns an empty string if the pattern matches but the hash specifies no replacements" do
    "hello".gsub(/./, 'z' => 'L').should == ""
  end

  it "ignores non-String keys" do
    "tattoo".gsub(/(tt)/, 'tt' => 'b', tt: 'z').should == "taboo"
  end

  it "uses a key's value as many times as needed" do
    "food".gsub(/o/, 'o' => '0').should == "f00d"
  end

  it "uses the hash's default value for missing keys" do
    hsh = {}
    hsh.default='?'
    hsh['o'] = '0'
    "food".gsub(/./, hsh).should == "?00?"
  end

  it "coerces the hash values with #to_s" do
    hsh = {}
    hsh.default=[]
    hsh['o'] = 0
    obj = mock('!')
    obj.should_receive(:to_s).and_return('!')
    hsh['!'] = obj
    "food!".gsub(/./, hsh).should == "[]00[]!"
  end

  it "uses the hash's value set from default_proc for missing keys" do
    hsh = {}
    hsh.default_proc = -> k, v { 'lamb' }
    "food!".gsub(/./, hsh).should == "lamblamblamblamblamb"
  end

  it "sets $~ to MatchData of last match and nil when there's none for access from outside" do
    NATFIXME 'Support hash argument', exception: SpecFailedException do
      'hello.'.gsub('l', 'l' => 'L')
      $~.begin(0).should == 3
      $~[0].should == 'l'

      'hello.'.gsub('not', 'ot' => 'to')
      $~.should == nil

      'hello.'.gsub(/.(.)/, 'o' => ' hole')
      $~[0].should == 'o.'

      'hello.'.gsub(/not/, 'z' => 'glark')
      $~.should == nil
    end
  end

  it "doesn't interpolate special sequences like \\1 for the block's return value" do
    repl = '\& \0 \1 \` \\\' \+ \\\\ foo'
    "hello".gsub(/(.+)/, 'hello' => repl ).should == repl
  end
end

describe "String#gsub! with pattern and Hash" do

  it "returns self with all occurrences of pattern replaced with the value of the corresponding hash key" do
    "hello".gsub!(/./, 'l' => 'L').should == "LL"
    "hello!".gsub!(/(.)(.)/, 'he' => 'she ', 'll' => 'said').should == 'she said'
    "hello".gsub!('l', 'l' => 'el').should == 'heelelo'
  end

  it "ignores keys that don't correspond to matches" do
    "hello".gsub!(/./, 'z' => 'L', 'h' => 'b', 'o' => 'ow').should == "bow"
  end

  it "replaces self with an empty string if the pattern matches but the hash specifies no replacements" do
    "hello".gsub!(/./, 'z' => 'L').should == ""
  end

  it "ignores non-String keys" do
    "hello".gsub!(/(ll)/, 'll' => 'r', ll: 'z').should == "hero"
  end

  it "uses a key's value as many times as needed" do
    "food".gsub!(/o/, 'o' => '0').should == "f00d"
  end

  it "uses the hash's default value for missing keys" do
    hsh = {}
    hsh.default='?'
    hsh['o'] = '0'
    "food".gsub!(/./, hsh).should == "?00?"
  end

  it "coerces the hash values with #to_s" do
    hsh = {}
    hsh.default=[]
    hsh['o'] = 0
    obj = mock('!')
    obj.should_receive(:to_s).and_return('!')
    hsh['!'] = obj
    "food!".gsub!(/./, hsh).should == "[]00[]!"
  end

  it "uses the hash's value set from default_proc for missing keys" do
    hsh = {}
    hsh.default_proc = -> k, v { 'lamb' }
    "food!".gsub!(/./, hsh).should == "lamblamblamblamblamb"
  end

  it "sets $~ to MatchData of last match and nil when there's none for access from outside" do
    NATFIXME 'Support hash argument', exception: SpecFailedException do
      'hello.'.gsub!('l', 'l' => 'L')
      $~.begin(0).should == 3
      $~[0].should == 'l'

      'hello.'.gsub!('not', 'ot' => 'to')
      $~.should == nil

      'hello.'.gsub!(/.(.)/, 'o' => ' hole')
      $~[0].should == 'o.'

      'hello.'.gsub!(/not/, 'z' => 'glark')
      $~.should == nil
    end
  end

  it "doesn't interpolate special sequences like \\1 for the block's return value" do
    repl = '\& \0 \1 \` \\\' \+ \\\\ foo'
    "hello".gsub!(/(.+)/, 'hello' => repl ).should == repl
  end
end

describe "String#gsub with pattern and block" do
  it "returns a copy of self with all occurrences of pattern replaced with the block's return value" do
    "hello".gsub(/./) { |s| s.succ + ' ' }.should == "i f m m p "
    "hello!".gsub(/(.)(.)/) { |*a| a.inspect }.should == '["he"]["ll"]["o!"]'
    "hello".gsub('l') { 'x'}.should == 'hexxo'
  end

  it "sets $~ for access from the block" do
    str = "hello"
    str.gsub(/([aeiou])/) { "<#{$~[1]}>" }.should == "h<e>ll<o>"
    str.gsub(/([aeiou])/) { "<#{$1}>" }.should == "h<e>ll<o>"
    str.gsub("l") { "<#{$~[0]}>" }.should == "he<l><l>o"

    offsets = []

    str.gsub(/([aeiou])/) do
      md = $~
      md.string.should == str
      offsets << md.offset(0)
      str
    end.should == "hhellollhello"

    offsets.should == [[1, 2], [4, 5]]
  end

  it "does not set $~ for procs created from methods" do
    NATFIXME 'proc from method handling', exception: SpecFailedException do
      str = "hello"
      str.gsub("l", &StringSpecs::SpecialVarProcessor.new.method(:process)).should == "he<unset><unset>o"
    end
  end

  it "restores $~ after leaving the block" do
    [/./, "l"].each do |pattern|
      old_md = nil
      "hello".gsub(pattern) do
        old_md = $~
        "ok".match(/./)
        "x"
      end

      $~[0].should == old_md[0]
      $~.string.should == "hello"
    end
  end

  it "sets $~ to MatchData of last match and nil when there's none for access from outside" do
    'hello.'.gsub('l') { 'x' }
    NATFIXME 'Implement $~', exception: NoMethodError, message: /undefined method [`']begin' for nil/ do
      $~.begin(0).should == 3
      $~[0].should == 'l'

      'hello.'.gsub('not') { 'x' }
      $~.should == nil

      'hello.'.gsub(/.(.)/) { 'x' }
      $~[0].should == 'o.'

      'hello.'.gsub(/not/) { 'x' }
      $~.should == nil
    end
  end

  it "doesn't interpolate special sequences like \\1 for the block's return value" do
    repl = '\& \0 \1 \` \\\' \+ \\\\ foo'
    "hello".gsub(/(.+)/) { repl }.should == repl
  end

  it "converts the block's return value to a string using to_s" do
    replacement = mock('hello_replacement')
    def replacement.to_s() "hello_replacement" end

    "hello".gsub(/hello/) { replacement }.should == "hello_replacement"

    obj = mock('ok')
    def obj.to_s() "ok" end

    "hello".gsub(/.+/) { obj }.should == "ok"
  end

  it "uses the compatible encoding if they are compatible" do
    s  = "hello"
    s2 = "#{195.chr}#{192.chr}#{195.chr}"

    NATFIXME 'uses the compatible encoding if they are compatible', exception: SpecFailedException do
      s.gsub(/l/) { |bar| 195.chr }.encoding.should == Encoding::BINARY
      s2.gsub("#{192.chr}") { |bar| "hello" }.encoding.should == Encoding::BINARY
    end
  end

  it "raises an Encoding::CompatibilityError if the encodings are not compatible" do
    s = "hllëllo"
    s2 = "hellö"

    NATFIXME 'Raise Encoding::CompatibilityError', exception: SpecFailedException do
      -> { s.gsub(/l/) { |bar| "Русский".force_encoding("iso-8859-5") } }.should raise_error(Encoding::CompatibilityError)
      -> { s2.gsub(/l/) { |bar| "Русский".force_encoding("iso-8859-5") } }.should raise_error(Encoding::CompatibilityError)
    end
  end

  it "replaces the incompatible part properly even if the encodings are not compatible" do
    s = "hllëllo"

    NATFIXME 'Not preserving encoding properly', exception: SpecFailedException do
      s.gsub(/ë/) { |bar| "Русский".force_encoding("iso-8859-5") }.encoding.should == Encoding::ISO_8859_5
    end
  end

  not_supported_on :opal do
    it "raises an ArgumentError if encoding is not valid" do
      x92 = [0x92].pack('C').force_encoding('utf-8')
      NATFIXME 'raises an ArgumentError if encoding is not valid', exception: SpecFailedException do
        -> { "a#{x92}b".gsub(/[^\x00-\x7f]/u, '') }.should raise_error(ArgumentError)
      end
    end
  end
end

describe "String#gsub with pattern and without replacement and block" do
  it "returns an enumerator" do
    NATFIXME 'Enumerator reply in String#gsub', exception: NotImplementedError do
      enum = "abca".gsub(/a/)
      enum.should be_an_instance_of(Enumerator)
      enum.to_a.should == ["a", "a"]
    end
  end

  describe "returned Enumerator" do
    describe "size" do
      it "should return nil" do
        NATFIXME 'Enumerator reply in String#gsub', exception: NotImplementedError do
          "abca".gsub(/a/).size.should == nil
        end
      end
    end
  end
end

describe "String#gsub with a string pattern" do
  it "handles multibyte characters" do
    "é".gsub("é", "â").should == "â"
    "aé".gsub("é", "â").should == "aâ"
    "éa".gsub("é", "â").should == "âa"
  end
end

describe "String#gsub! with pattern and replacement" do
  it "modifies self in place and returns self" do
    a = "hello"
    a.gsub!(/[aeiou]/, '*').should equal(a)
    a.should == "h*ll*"
  end

  it "modifies self in place with multi-byte characters and returns self" do
    a = "¿por qué?"
    a.gsub!(/([a-z\d]*)/, "*").should equal(a)
    a.should == "*¿** **é*?*"
  end

  it "returns nil if no modifications were made" do
    a = "hello"
    a.gsub!(/z/, '*').should == nil
    a.gsub!(/z/, 'z').should == nil
    a.should == "hello"
  end

  # See [ruby-core:23666]
  it "raises a FrozenError when self is frozen" do
    s = "hello"
    s.freeze

    -> { s.gsub!(/ROAR/, "x")    }.should raise_error(FrozenError)
    -> { s.gsub!(/e/, "e")       }.should raise_error(FrozenError)
    -> { s.gsub!(/[aeiou]/, '*') }.should raise_error(FrozenError)
  end

  it "handles a pattern in a superset encoding" do
    string = 'abc'.force_encoding(Encoding::US_ASCII)

    result = string.gsub!('é', 'è')

    result.should == nil
    string.should == 'abc'
    string.encoding.should == Encoding::US_ASCII
  end

  it "handles a pattern in a subset encoding" do
    string = 'été'
    pattern = 't'.force_encoding(Encoding::US_ASCII)

    result = string.gsub!(pattern, 'u')

    result.should == string
    string.should == 'éué'
    string.encoding.should == Encoding::UTF_8
  end
end

describe "String#gsub! with pattern and block" do
  it "modifies self in place and returns self" do
    a = "hello"
    a.gsub!(/[aeiou]/) { '*' }.should equal(a)
    a.should == "h*ll*"
  end

  it "returns nil if no modifications were made" do
    a = "hello"
    a.gsub!(/z/) { '*' }.should == nil
    a.gsub!(/z/) { 'z' }.should == nil
    a.should == "hello"
  end

  # See [ruby-core:23663]
  it "raises a FrozenError when self is frozen" do
    s = "hello"
    s.freeze

    -> { s.gsub!(/ROAR/)    { "x" } }.should raise_error(FrozenError)
    -> { s.gsub!(/e/)       { "e" } }.should raise_error(FrozenError)
    -> { s.gsub!(/[aeiou]/) { '*' } }.should raise_error(FrozenError)
  end

  it "uses the compatible encoding if they are compatible" do
    s  = "hello"
    s2 = "#{195.chr}#{192.chr}#{195.chr}"

    NATFIXME 'Encodings', exception: SpecFailedException do
      s.gsub!(/l/) { |bar| 195.chr }.encoding.should == Encoding::BINARY
      s2.gsub!("#{192.chr}") { |bar| "hello" }.encoding.should == Encoding::BINARY
    end
  end

  it "raises an Encoding::CompatibilityError if the encodings are not compatible" do
    s = "hllëllo"
    s2 = "hellö"

    NATFIXME 'Raise Encoding::CompatibilityError', exception: SpecFailedException do
      -> { s.gsub!(/l/) { |bar| "Русский".force_encoding("iso-8859-5") } }.should raise_error(Encoding::CompatibilityError)
      -> { s2.gsub!(/l/) { |bar| "Русский".force_encoding("iso-8859-5") } }.should raise_error(Encoding::CompatibilityError)
    end
  end

  it "replaces the incompatible part properly even if the encodings are not compatible" do
    s = "hllëllo"

    NATFIXME 'Encodings', exception: SpecFailedException do
      s.gsub!(/ë/) { |bar| "Русский".force_encoding("iso-8859-5") }.encoding.should == Encoding::ISO_8859_5
    end
  end

  not_supported_on :opal do
    it "raises an ArgumentError if encoding is not valid" do
      x92 = [0x92].pack('C').force_encoding('utf-8')
      NATFIXME 'Encodings', exception: SpecFailedException do
        -> { "a#{x92}b".gsub!(/[^\x00-\x7f]/u, '') }.should raise_error(ArgumentError)
      end
    end
  end
end

describe "String#gsub! with pattern and without replacement and block" do
  it "returns an enumerator" do
    NATFIXME 'Enumerator reply in String#gsub!', exception: NotImplementedError do
      enum = "abca".gsub!(/a/)
      enum.should be_an_instance_of(Enumerator)
      enum.to_a.should == ["a", "a"]
    end
  end

  describe "returned Enumerator" do
    describe "size" do
      it "should return nil" do
        NATFIXME 'Enumerator reply in String#gsub!', exception: NotImplementedError do
          "abca".gsub!(/a/).size.should == nil
        end
      end
    end
  end
end
