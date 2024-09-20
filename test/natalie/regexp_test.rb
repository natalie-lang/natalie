require_relative '../spec_helper'

describe 'regexp' do
  it 'can be created with syntax' do
    r = /foo/
    r.should be_kind_of(Regexp)
    r = /foo/ixm
    r.should be_kind_of(Regexp)
    r.inspect.should == '/foo/mix'
  end

  it 'can be created dynamically' do
    r = /foo #{1 + 1}/
    r.should be_kind_of(Regexp)
    r.inspect.should == '/foo 2/'
  end

  it 'can handle the # character with or without string interpolation' do
    /\#foo/.should =~ '#foo'
    /#foo/.should =~ '#foo'
    /#$1/.should =~ '#$1'
    @bar = 'x'
    /#@bar/.should =~ 'x'
    /\#@bar/.should =~ '#@bar'
  end

  it 'can handle bracketed unicode escape sequences' do
    /\n/.source.should == '\n'
    /\u0000/.source.should == '\u0000'
    /\u1234/.source.should == '\u1234'
    /\x20/.source.should == '\x20'

    r = /\u{1}/;      r.source.should == '\u{1}';      r.should =~ "\u{1}"
    r = /\u{1F}/;     r.source.should == '\u{1F}';     r.should =~ "\u{1F}"
    r = /\u{1fa}/;    r.source.should == '\u{1fa}';    r.should =~ "\u{1fa}"
    r = /\u{aEaD}/;   r.source.should == '\u{aEaD}';   r.should =~ "\u{aEaD}"
    r = /\u{f2345}/;  r.source.should == '\u{f2345}';  r.should =~ "\u{f2345}"
    r = /\u{10FFFF}/; r.source.should == '\u{10FFFF}'; r.should =~ "\u{10FFFF}"

    /^\u{1F90F  1F3FC}$/.should =~ [
      240, 159, 164, 143, 240, 159, 143, 188
    ].pack('C*').force_encoding(Encoding::UTF_8)

    -> { eval('/\u{1111111}/') }.should raise_error(SyntaxError)

    -> { eval('/\u{}/') }.should raise_error(SyntaxError)
    -> { eval('/\u{111111}/') }.should raise_error(SyntaxError)
  end

  it 'uses the right onigmo encoding' do
    pattern = Regexp.new("木".encode('EUC-JP'), Regexp::FIXEDENCODING)
    pattern.encoding.should == Encoding::EUC_JP
  end

  it 'can embed regexp that ignores multiline and comments' do
    r1 = /
    foo
    /x
    r1.should =~ 'foo'

    r2 = /\A#{r1}\z/
    r2.should =~ 'foo'
  end

  describe '.new' do
    it 'can be created with a string or another regexp' do
      r1 = Regexp.new('foo.*bar')
      r1.should be_kind_of(Regexp)
      r2 = Regexp.new(r1)
      r2.should be_kind_of(Regexp)
      r1.should == r2
    end

    it 'automatically applies FIXEDENCODING flag' do
      r = Regexp.new('\x80', Regexp::NOENCODING)
      r.options.should == Regexp::FIXEDENCODING | Regexp::NOENCODING
      r.to_s.should == '(?-mix:\x80)'

      r = Regexp.new('\u1234', 0)
      r.options.should == Regexp::FIXEDENCODING
    end

    it 'recognizes character properties' do
      Regexp.new("\\p{L}").should be_an_instance_of(Regexp)
      Regexp.new("\\p{Arabic}").should be_an_instance_of(Regexp)
      Regexp.new("\\p{ Arabic }").should be_an_instance_of(Regexp)
    end

    it 'raises a RegexpError for bad character properties' do
      -> { Regexp.new('\p{}') }.should raise_error(RegexpError)
      -> { Regexp.new('\p{zzz}') }.should raise_error(RegexpError)
      -> { Regexp.new('\p{') }.should raise_error(RegexpError)
      -> { Regexp.new('\p{ ') }.should raise_error(RegexpError)
    end
  end

  describe '#==' do
    it 'returns true if the regexp source is the same' do
      /foo/.should == /foo/
    end

    it 'returns false if the regexp source is different' do
      /foo/.should != /food/
    end
  end

  describe '#inspect' do
    it 'returns a string representation' do
      r = /foo/
      r.inspect.should == '/foo/'
    end

    it 'does not escape tabs or newlines' do
      str = "\n\tfoo\n"
      r = Regexp.new(str, Regexp::EXTENDED)
      r.inspect.should == "/#{str}/x"
    end
  end

  describe '#to_s' do
    it 'does not escape tabs or newlines' do
      str = "\n\tfoo\n"
      r = Regexp.new(str, Regexp::EXTENDED)
      r.to_s.should == "(?x-mi:#{str})"
    end
  end

  describe '#match' do
    it 'works' do
      match = /foo/.match('foo')
      match.should be_kind_of(MatchData)
      match[0].should == 'foo'
    end

    it 'accepts a start argument' do
      match = /foo/.match('😊😊foo bar foo', 4)
      match.should be_kind_of(MatchData)
      match.begin(0).should == 10

      match = /foo/.match('foo bar foo😊', -4)
      match.should be_kind_of(MatchData)
      match.begin(0).should == 8
    end
  end

  describe '#match on String' do
    it 'works' do
      match = 'foo'.match(/foo/)
      match.should be_kind_of(MatchData)
      match[0].should == 'foo'
    end

    it 'accepts a start argument' do
      match = '😊😊foo bar foo'.match(/foo/, 4)
      match.should be_kind_of(MatchData)
      match.begin(0).should == 10

      match = 'foo bar foo😊'.match(/foo/, -4)
      match.should be_kind_of(MatchData)
      match.begin(0).should == 8
    end
  end

  describe '=~' do
    it 'return an integer for match' do
      result = /foo/ =~ 'foo'
      result.should == 0
      result = /bar/ =~ 'foo bar'
      result.should == 4
    end

    it 'return nil for no match' do
      result = /foo/ =~ 'bar'
      result.should == nil
    end

    it 'works with symbols' do
      result = /foo/ =~ :foo
      result.should == 0
    end
  end

  describe '===' do
    it 'works with strings' do
      result = /foo/ === 'foo'
      result.should == true
    end

    it 'works with symbols' do
      result = /foo/ === :foo
      result.should == true
    end
  end

  describe '!~' do
    it 'return a boolean for match' do
      result = /foo/ !~ 'foo'
      result.should == false
      result = /bar/ !~ 'baz'
      result.should == true
    end
  end

  describe '=~ on String' do
    it 'works' do
      result = 'foo' =~ /foo/
      result.should == 0
    end

    it 'works with escaped characters' do
      result = '*name' =~ /^\*(.+)/
      result.should == 0
      result = 'n' =~ /^\*(.+)/
      result.should be_nil
    end
  end

  describe 'magic global variables' do
    it 'returns the most recent match' do
      'tim' =~ /t(i)(m)/
      $&.should == 'tim'
      $~.to_s.should == 'tim'
      $1.should == 'i'
      $2.should == 'm'
      $3.should == nil
    end

    it 'returns the most recent match from a block' do
      [1].each { [1].each { 'tim' =~ /t(i)(m)/ } }
      [1].each do
        $&.should == 'tim'
        $~.to_s.should == 'tim'
        $1.should == 'i'
        $2.should == 'm'
        $3.should == nil
      end
    end

    it 'does not return the most recent match from a different method' do
      '' =~ /xxx/ # reset match
      def m1
        'tim' =~ /t(i)(m)/
      end
      m1
      $&.should be_nil
      $~.should be_nil
      $1.should be_nil
      $2.should be_nil
      $3.should be_nil

      '' =~ /xxx/ # reset match
      def m2
        'tim' =~ /t(i)(m)/
      end
      def m3
        $&.should be_nil
        $~.should be_nil
        $1.should be_nil
        $2.should be_nil
        $3.should be_nil
      end
      m2
      m3

      'tim' =~ /t(i)(m)/
      [1].each { [1].each { 'tim' =~ /t(i)(m)/ } }
      def m4
        $&.should be_nil
        $~.should be_nil
        $1.should be_nil
        $2.should be_nil
        $3.should be_nil
      end
      m4
    end
  end

  describe '.compile' do
    it 'creates a regexp from a string' do
      r = Regexp.compile('t(i)m+', true)
      r.should == /t(i)m+/i
    end

    it 'creates a regexp from a string with options' do
      r = Regexp.compile('tim', Regexp::EXTENDED | Regexp::IGNORECASE)
      r.should == /tim/ix
    end
  end

  describe '#source' do
    it 'returns the literal representation of the regexp' do
      r = /^foo\n[0-9]+ (bar){1,2}$/
      r.source.should == "^foo\\n[0-9]+ (bar){1,2}$"
    end
  end

  describe '#options' do
    it 'returns the literal representation of the regexp' do
      r = /^foo\n[0-9]+ (bar){1,2}$/ixm
      r.options.should == 7
    end
  end

  describe '#gsub' do
    it 'does not get stuck in a loop if the replacement is larger than the pattern and matches again' do
      'f'.gsub(/f*/, 'ffff').should == 'ffffffff'
    end

    it 'works with zero-width match' do
      "f\nmp".gsub(/^/, 'gi').should == "gif\ngimp"
    end

    it 'works with a null byte' do
      "foo\0bar\0baz".gsub(/\0/, '?').should == 'foo?bar?baz'
    end
  end

  describe '#sub' do
    it 'expands backrefs' do
      'tim'.sub(/t(i)m/, "\\1").should == 'i'
      'tim'.sub(/t(i)m/, "\\9").should == ''
      'tim'.sub(/t(i)m/, "0\\10").should == '0i0'
      'tim'.sub(/t(i)m/, "0\\90").should == '00'
    end

    it 'works with a null byte' do
      "foo\0bar\0baz".sub(/\0/, '?').should == "foo?bar\0baz"
    end
  end

  describe 'named capture' do
    it 'sets variables when there is a match' do
      /(?<name>\w+) is (?'age'\d+) years/ =~ 'Joe is 21 years old'
      name.should == 'Joe'
      age.should == '21'
    end

    it 'works with hyphenated name' do
      m = /(?<name-and-age>\w+ is \d+)/.match('Joe is 21 years old')
      m.named_captures['name-and-age'].should == 'Joe is 21'
    end


    it 'sets variables to nil when there is no match' do
      /(?<name>\w+) is (?'age'\d+) years/ =~ 'Joe is not yet 21 years old'
      name.should == nil
      age.should == nil
    end

    it 'does affect outer scope' do
      name = 'before'
      nil.tap do
        /(?<name>\w+)/ =~ 'Joe'
        name.should == 'Joe'
      end
      name.should == 'Joe'
    end
  end
end
