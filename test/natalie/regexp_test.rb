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

  describe '.new' do
    it 'can be created with a string or another regexp' do
      r1 = Regexp.new('foo.*bar')
      r1.should be_kind_of(Regexp)
      r2 = Regexp.new(r1)
      r2.should be_kind_of(Regexp)
      r1.should == r2
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

  describe '#match' do
    describe '#size' do
      it 'returns the number of captures (including the whole match)' do
        match = /foo/.match('foo')
        match.size.should == 1
        match = /bar/.match('foo bar baz')
        match.size.should == 1
        match = /bar (baz)/.match('foo bar baz')
        match.size.should == 2
      end
    end

    describe '#length' do
      it 'returns the number of captures (including the whole match)' do
        match = /foo/.match('foo')
        match.length.should == 1
        match = /bar/.match('foo bar baz')
        match.length.should == 1
        match = /bar (baz)/.match('foo bar baz')
        match.length.should == 2
      end
    end

    describe '#to_s' do
      it 'returns the matched string' do
        match = /foo/.match('foo')
        match.to_s.should == 'foo'
        match = /bar/.match('foo bar baz')
        match.to_s.should == 'bar'
        match = /bar (baz)/.match('foo bar baz')
        match.to_s.should == 'bar baz'
      end
    end

    describe '#[]' do
      it 'returns a capture group' do
        match = /(foo) bar (baz)/.match('foo bar baz')
        match[0].should == 'foo bar baz'
        match[1].should == 'foo'
        match[2].should == 'baz'
        match[3].should == nil
      end
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

  describe '#match on String' do
    it 'works' do
      match = 'foo'.match(/foo/)
      match.should be_kind_of(MatchData)
      match[0].should == 'foo'
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
end
