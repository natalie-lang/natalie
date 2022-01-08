# encoding: UTF-8

require_relative '../spec_helper'

describe 'string' do
  it 'processes backslashes properly' do
    "foo\\bar".should == 'foo' + "\\" + 'bar'
  end

  describe '#inspect' do
    it 'returns a code representation of the string' do
      'foo'.inspect.should == '"foo"'
      "foo\nbar".inspect.should == "\"foo\\nbar\""
      'foo#bar'.inspect.should == '"foo#bar"'
      'foo#{1+1}'.inspect.should == '"foo\\#{1+1}"'
      "foo\x1f".encode('utf-8').inspect.should == '"foo\\u001F"'
      "foo\x00\x04".encode('utf-8').inspect.should == '"foo\\u0000\\u0004"'
      "foo\x00\x04".encode('ascii-8bit').inspect.should == '"foo\\x00\\x04"'
      '😉🤷'.inspect.should == "\"😉🤷\"" unless RUBY_PLATFORM =~ /openbsd/
    end
  end

  describe '#size' do
    it 'returns the number of characters in the string' do
      '😉😉😉'.encode('utf-8').size.should == 3
      'foo bar'.size.should == 7
    end
  end

  describe '#length' do
    it 'returns the number of characters in the string' do
      '😉😉😉'.encode('utf-8').size.should == 3
      'foo bar'.size.should == 7
    end
  end

  describe '#<=>' do
    it 'should return -1 if lhs is less than rhs' do
      ('a' <=> 'b').should == -1
      ('a' <=> 'z').should == -1
    end

    it 'should return 1 if lhs is greater than rhs' do
      ('b' <=> 'a').should == 1
      ('z' <=> 'a').should == 1
    end

    it 'should return 0 if both sides are equal' do
      ('a' <=> 'a').should == 0
      ('z' <=> 'z').should == 0
    end
  end

  describe '#<' do
    it 'returns true if lhs is alphabetically less than rhs' do
      ('a' < 'b').should == true
      ('a' < 'a').should == false
      ('z' < 'aa').should == false
    end
  end

  describe '#<=' do
    it 'returns true if lhs is alphabetically less than rhs' do
      ('a' <= 'b').should == true
      ('a' <= 'a').should == true
      ('z' <= 'aa').should == false
    end
  end

  describe '#bytes' do
    it 'returns an array of byte values' do
      'foo'.bytes.should == [102, 111, 111]
    end
  end

  describe '#ord' do
    it 'returns the character code for the first character of the string' do
      ' '.ord.should == 32
      'a'.ord.should == 97
      'abc'.ord.should == 97
      'ă'.encode('utf-8').ord.should == 259
      '”'.encode('utf-8').ord.should == 8221
      '😉'.encode('utf-8').ord.should == 128_521
      '😉😉😉'.encode('utf-8').ord.should == 128_521
    end

    it 'raises an error if the string is empty' do
      -> { ''.ord }.should raise_error(ArgumentError, 'empty string')
    end
  end

  describe '#encode' do
    it 'changes the encoding while reinterpreting the characters' do
      s = 'abc123'.encode 'utf-8'
      s.encoding.should == Encoding::UTF_8
      s2 = s.encode 'ascii-8bit'
      s2.encoding.should == Encoding::ASCII_8BIT
      s2.should == 'abc123'
      s3 = s2.encode 'utf-8'
      s3.encoding.should == Encoding::UTF_8
      s3.should == 'abc123'
    end

    it 'raises an error if a character cannot be converted to the new encoding' do
      s = 'abc 😢'.encode 'utf-8'
      s.encoding.should == Encoding::UTF_8
      -> { s.encode 'ascii-8bit' }.should raise_error(
                                            Encoding::UndefinedConversionError,
                                            'U+1F622 from UTF-8 to ASCII-8BIT',
                                          )
      s = 'xyz 🥺'.encode 'utf-8'
      s.encoding.should == Encoding::UTF_8
      -> { s.encode 'ascii-8bit' }.should raise_error(
                                            Encoding::UndefinedConversionError,
                                            'U+1F97A from UTF-8 to ASCII-8BIT',
                                          )
    end

    it 'raises an error if the encoding converter does not exist' do
      s = 'abc 😢'.encode 'utf-8'
      -> { s.encode 'bogus-fake-encoding' }.should raise_error(StandardError) # TODO: not actually the right error ;-)
    end
  end

  describe '#force_encoding' do
    it 'changes the encoding without reinterpreting the characters' do
      s = ''.encode 'utf-8'
      s.encoding.should == Encoding::UTF_8
      s.force_encoding 'ascii-8bit'
      s.encoding.should == Encoding::ASCII_8BIT
    end
  end

  describe '#each_char' do
    it 'yields to the block each character' do
      result = []
      'foo'.each_char { |char| result << char }
      result.should == %w[f o o]
    end
  end

  describe '#chars' do
    it 'returns an array of characters' do
      'foo'.chars.should == %w[f o o]
      s = '😉”ăa'.encode 'utf-8'
      s.chars.should == %w[😉 ” ă a]
      s.force_encoding 'ascii-8bit'
      s.chars.map { |c| c.ord }.should == [240, 159, 152, 137, 226, 128, 157, 196, 131, 97]
    end
  end

  describe '#[]' do
    it 'returns the character at the given index' do
      s = '😉”ăa'.encode 'utf-8'
      s[0].should == '😉'
      s[1].should == '”'
      s[2].should == 'ă'
      s[3].should == 'a'
    end

    it 'returns nil if the index is past the end' do
      s = '😉”ăa'.encode 'utf-8'
      s[4].should == nil
    end

    it 'returns the character from the end given a negative index' do
      s = '😉”ăa'.encode 'utf-8'
      s[-1].should == 'a'
      s[-2].should == 'ă'
      s[-3].should == '”'
      s[-4].should == '😉'
    end

    it 'returns nil if the negative index is too small' do
      s = '😉”ăa'.encode 'utf-8'
      s[-5].should == nil
    end

    context 'given a range' do
      it 'returns a substring' do
        s = '😉”ăa'.encode 'utf-8'
        s[1..-1].should == '”ăa'
        s = 'hello'
        s[1..-1].should == 'ello'
        s = 'n'
        s[1..-1].should == ''
      end

      it 'returns proper result for a range out of bounds' do
        s = 'hello'
        s[-2..0].should == ''
        s[2..100].should == 'llo'
      end

      it 'returns nil for a range that starts beyond the end of the string' do
        s = 'hello'
        s[90..100].should == nil
      end
    end
  end

  describe '#succ' do
    context 'given a single character' do
      it 'returns the next character' do
        'a'.succ.should == 'b'
        'm'.succ.should == 'n'
        'A'.succ.should == 'B'
        'M'.succ.should == 'N'
        '0'.succ.should == '1'
      end

      it 'loops on z' do
        'z'.succ.should == 'aa'
      end

      it 'loops on Z' do
        'Z'.succ.should == 'AA'
      end

      it 'loops on 9' do
        '9'.succ.should == '10'
      end
    end

    context 'given multiple characters' do
      it 'loops on z' do
        'az'.succ.should == 'ba'
        'aaz'.succ.should == 'aba'
        'zzz'.succ.should == 'aaaa'
      end
    end # TODO: handle mixed case, e.g. 'Az' and 'Zz'

    context 'given a character outside alphanumeric range' do
      it 'returns the next character' do
        '👍'.succ.should == '👎'
      end
    end
  end

  describe '#index' do
    it 'returns the character index of the substring' do
      s = 'tim is 😉 ok'.encode 'utf-8'
      s.index('tim').should == 0
      s.index('is').should == 4
      s.index('😉').should == 7
      s.index('ok').should == 9
    end

    it 'returns nil if the substring cannot be found' do
      s = 'tim is ok'
      s.index('rocks').should == nil
    end
  end

  describe '#start_with?' do
    it 'returns true if the string starts with the given substring' do
      s = 'tim morgan'
      s.start_with?('tim').should be_true
      s.start_with?('t').should be_true
      s.start_with?('').should be_true
      s.start_with?('x').should be_false
      s.start_with?('xxxxxxxxxxxxxxx').should be_false
    end
  end

  describe '#end_with?' do
    it 'returns true if the string ends with the given substring' do
      s = 'tim morgan'
      s.end_with?('tim morgan').should be_true
      s.end_with?('morgan').should be_true
      s.end_with?('n').should be_true
      s.end_with?('').should be_true
      s.end_with?('x').should be_false
      s.end_with?('xxxxxxxxxxxxxxx').should be_false
    end
  end

  describe '#empty?' do
    it 'returns true if the string has a length of 0' do
      ''.empty?.should be_true
      'x'.empty?.should be_false
    end
  end

  describe '#sub' do
    it 'returns a duped string if no substitution was made' do
      s = 'tim is ok'
      s.sub('is cool', '').object_id.should != s.object_id
      s.sub(/is cool/, '').object_id.should != s.object_id
    end

    it 'replaces the matching string' do
      s = 'tim is ok'
      s.sub('is ok', 'rocks').should == 'tim rocks'
      s.should == 'tim is ok'
      s.sub(' is ok', '').should == 'tim'
      s.sub('bogus', '').should == 'tim is ok'
    end

    it 'replaces the matching regex' do
      s = 'tim is ok'
      s.sub(/is ok/, 'rocks').should == 'tim rocks'
      s.should == 'tim is ok'
      s.sub(/ is ok/, '').should == 'tim'
      s.sub(/is.*/, 'rocks').should == 'tim rocks'
      s.sub(/bogus/, '').should == 'tim is ok'
    end

    it 'substitues back references' do
      '0b1101011'.sub(/0b([01]+)/, 'the binary number is \1').should == 'the binary number is 1101011'
      'abc'.sub(/([a-z]+)/, '\0def').should == 'abcdef'
    end

    it 'raises an error if the arguments are of the wrong type' do
      -> { 'foo'.sub(1, 'bar') }.should raise_error(TypeError, 'wrong argument type Integer (expected Regexp)')
      -> { 'foo'.sub(:foo, 'bar') }.should raise_error(TypeError, 'wrong argument type Symbol (expected Regexp)')
      -> { 'foo'.sub('foo', :bar) }.should raise_error(TypeError, 'no implicit conversion of Symbol into String')
    end

    it 'replaces with the result of calling the block if a block is given' do
      'ruby is fun'.sub('ruby') { 'RUBY' }.should == 'RUBY is fun' # TODO: accept block for regex sub #'ruby is fun'.sub(/ruby/) { 'RUBY' }.should == 'RUBY is fun' # TODO: pass match object into block #'ruby is fun'.sub(/ruby/) { |m| m.to_s.upcase }.should == 'RUBY is fun'
    end
  end

  describe '#gsub' do
    it 'returns a copy of the string with all matching substrings replaced' do
      s = 'foo bar foo bar'
      s.gsub(/foo/, 'bar').should == 'bar bar bar bar'
      s.gsub(/foo/, 'foo foo').should == 'foo foo bar foo foo bar'
      s.gsub(/bar/, 'foo').should == 'foo foo foo foo'
      'abc'.gsub(/([a-z])/, '\0-').should == 'a-b-c-'
    end
  end

  describe '#to_i' do
    it 'returns an Integer by recognizing digits in the string' do
      '12345'.to_i.should == 12_345
      ' 12345'.to_i.should == 12_345
      ' 123 45'.to_i.should == 123
      '99 red balloons'.to_i.should == 99
      '0a'.to_i.should == 0
      '0a'.to_i(16).should == 10
      '0A'.to_i(16).should == 10
      'hello'.to_i.should == 0
      '1100101'.to_i(2).should == 101
      '1100101'.to_i(8).should == 294_977
      '1100101'.to_i(10).should == 1_100_101
      '1100101'.to_i(16).should == 17_826_049
    end
  end

  describe '#split' do
    it 'splits a string into an array of smaller strings using a string match' do
      ''.split(',').should == []
      ' '.split(',').should == [' ']
      'tim'.split(',').should == ['tim']
      'tim,morgan,rocks'.split(',').should == %w[tim morgan rocks]
      'tim morgan rocks'.split(' morgan ').should == %w[tim rocks]
    end

    it 'splits a string into an array of smaller strings using a regexp match' do
      ''.split(/,/).should == []
      ' '.split(/,/).should == [' ']
      'tim'.split(/,/).should == ['tim']
      'tim,morgan,rocks'.split(/,/).should == %w[tim morgan rocks]
      'tim     morgan rocks'.split(/\s+/).should == %w[tim morgan rocks]
      'tim morgan rocks'.split(/ mo[a-z]+ /).should == %w[tim rocks]
    end

    it 'only splits into the specified number of pieces' do
      'tim,morgan,rocks'.split(/,/, 1).should == ['tim,morgan,rocks']
      'tim,morgan,rocks'.split(/,/, 2).should == %w[tim morgan,rocks]
      'tim,morgan,rocks'.split(',', 2).should == %w[tim morgan,rocks]
    end
  end

  describe '#ljust' do
    it 'returns a padded copy using spaces to reach the desired length' do
      s = 'tim'
      s.ljust(10).should == 'tim       '
    end

    it 'returns a padded copy using the given padstr to reach the desired length' do
      s = 'tim'
      s.ljust(10, 'x').should == 'timxxxxxxx'
      s.ljust(10, 'xy').should == 'timxyxyxyx'
    end

    it 'returns an unmodified copy if the string length is already the desired length' do
      s = 'tim morgan'
      s.ljust(10).should == 'tim morgan'
      s.ljust(5).should == 'tim morgan'
      s.ljust(5, 'x').should == 'tim morgan'
    end
  end

  describe '#strip' do
    it 'returns a copy of the string with all starting and ending spaces removed' do
      ''.strip.should == ''
      ' '.strip.should == ''
      '     '.strip.should == ''
      '    foo    '.strip.should == 'foo'
      " \n  foo  \t  ".strip.should == 'foo'
    end

    it 'makes a copy of the string if no changes were made' do
      s1 = 'foo'
      s2 = s1.strip
      s1.object_id.should_not == s2
    end
  end

  describe '#downcase' do
    it 'returns a copy of the string with all characters lower-cased' do
      'FoObAr'.downcase.should == 'foobar'
    end
  end

  describe '#upcase' do
    it 'returns a copy of the string with all characters upper-cased' do
      'FoObAr'.upcase.should == 'FOOBAR'
    end
  end

  describe 'subclass' do
    class NegativeString < String
      def initialize(s)
        super('not ' + s)
        @bar = 'bar'
      end
      attr_reader :bar
    end

    it 'works' do
      s = NegativeString.new('foo')
      s.should == 'not foo'
      s.bar.should == 'bar'
    end
  end

  describe '#reverse' do
    specify do
      ''.reverse.should == ''
      't'.reverse.should == 't'
      'tim'.reverse.should == 'mit'
    end
  end

  describe '#include?' do
    it 'works on strings containing null character' do
      "foo\x00bar".include?('bar').should == true
      'foo'.include?("foo\x00bar").should == false
    end
  end
end
