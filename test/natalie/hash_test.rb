require_relative '../spec_helper'

describe 'hash' do
  describe '.new' do
    it 'builds an empty hash' do
      Hash.new.should == {}
    end

    it 'builds a hash with a default value' do
      h = Hash.new(:bar)
      h[:foo].should == :bar
      h[:foo] = :baz
      h[:foo].should == :baz
    end

    it 'builds a hash with a default value block' do
      h = Hash.new { :buz }
      h[:foo].should == :buz
      h[:foo] = :baz
      h[:foo].should == :baz
    end

    it 'can persist default value via block' do
      h = Hash.new { |hash, key| hash[key] = 'rocks' }
      h.should == {}
      h[:foo].should == 'rocks'
      h.should == { foo: 'rocks' }
    end

    it 'does not duplicate the default value before returning' do
      default = { 1 => 2 }
      h = Hash.new(default)
      h[:foo].should == { 1 => 2 }
      h[:foo][3] = 4
      h[:foo].should == { 1 => 2, 3 => 4 }
    end
  end

  describe '.[]' do
    it 'returns an empty hash if given no args' do
      Hash[].should == {}
    end

    it 'returns a new hash if given sequential pairs' do
      h = Hash[1, 2, 3, 4]
      h.should == { 1 => 2, 3 => 4 }
    end

    it 'returns a hash if given an array of pairs' do
      h = Hash[[['a', 100], ['b', 200], ['c']]]
      h.should == { 'a' => 100, 'b' => 200, 'c' => nil }
    end

    it 'raises an error if given an array of pairs where a pair has the wrong number of items' do
      -> { Hash[[[]]] }.should raise_error(ArgumentError, 'invalid number of elements (0 for 1..2)')
      -> { Hash[[[1, 2, 3]]] }.should raise_error(ArgumentError, 'invalid number of elements (3 for 1..2)')
    end

    it 'raises an error if given an odd number of arguments' do
      -> { Hash[1] }.should raise_error(ArgumentError, 'odd number of arguments for Hash')
    end

    it 'returns the hash if given a hash' do
      h = Hash['a' => 100, 'b' => 200]
      h.should == { 'a' => 100, 'b' => 200 }
    end
  end

  describe '#sort' do
    it 'returns a sorted array of pairs' do
      h = { 2 => 3, 1 => 2 }
      h.sort.should == [[1, 2], [2, 3]]
    end
  end

  it 'can use any object as a key' do
    { 1 => 'val' }[1].should == 'val'
    { sym: 'val' }[:sym].should == 'val'
    { 'str' => 'val' }['str'].should == 'val'
    key = Object.new
    { key => 'val' }[key].should == 'val'
  end

  it 'maintains insertion order' do
    hash = { 1 => 1, 2 => 2, 3 => 3, 'four' => 4, :five => 5 }
    hash.inspect.should == '{1=>1, 2=>2, 3=>3, "four"=>4, :five=>5}'
    hash['six'] = 6
    hash.inspect.should == '{1=>1, 2=>2, 3=>3, "four"=>4, :five=>5, "six"=>6}'
    hash[2] = 'two'
    hash.inspect.should == '{1=>1, 2=>"two", 3=>3, "four"=>4, :five=>5, "six"=>6}'
    hash[2] = nil
    hash.inspect.should == '{1=>1, 2=>nil, 3=>3, "four"=>4, :five=>5, "six"=>6}'
    hash[2] = 'two'
    hash.inspect.should == '{1=>1, 2=>"two", 3=>3, "four"=>4, :five=>5, "six"=>6}'
    hash.delete(2)
    hash.inspect.should == '{1=>1, 3=>3, "four"=>4, :five=>5, "six"=>6}'
    hash[2] = 2
    hash.inspect.should == '{1=>1, 3=>3, "four"=>4, :five=>5, "six"=>6, 2=>2}'
  end

  it 'can delete the last key' do
    hash = {}
    hash.inspect.should == '{}'
    hash.size.should == 0
    hash[1] = 1
    hash.inspect.should == '{1=>1}'
    hash.size.should == 1
    hash.delete(1)
    hash.inspect.should == '{}'
    hash.size.should == 0
  end

  it 'can be compared for equality' do
    {}.should == {}
    { 1 => 1 }.should == { 1 => 1 }
    hash = { 1 => 'one', :two => 2, 'three' => 3 }
    hash.should == hash
    hash.should == { 1 => 'one', :two => 2, 'three' => 3 }
    hash.delete(1)
    hash.delete(:two)
    hash.delete('three')
    hash.should == {}
  end

  it 'can be iterated over' do
    hash = { 1 => 'one', 2 => 'two', 3 => 'three' }
    keys = []
    vals = []
    r =
      hash.each do |key, val|
        keys << key
        vals << val
      end
    keys.should == [1, 2, 3]
    vals.should == %w[one two three]
    r.should == hash
  end

  it 'raises an error when adding a new key while iterating' do
    hash = { 1 => 'one', 2 => 'two', 3 => 'three' }
    -> { hash.each { |key, val| hash[4] = 'four' } }.should raise_error(
                                                              RuntimeError,
                                                              "can't add a new key into hash during iteration",
                                                            )
  end

  it 'an entry can be modified during iteration' do
    hash = { 1 => 'one' }
    vals = []
    hash.each do |key, val|
      hash[1] = 'ONE'
      vals << val
    end
    vals.should == ['one']
    hash.should == { 1 => 'ONE' }
    hash.size.should == 1
  end

  it 'the only key can be deleted during iteration' do
    hash = { 1 => 'one' }
    vals = []
    hash.each do |key, val|
      hash.delete(1)
      vals << val
    end
    vals.should == ['one']
    hash.should == {}
    hash.size.should == 0
  end

  it 'the current key can be deleted during iteration' do
    hash = { 1 => 'one', 2 => 'two', 3 => 'three' }
    vals = []
    hash.each do |key, val|
      hash.delete(1)
      vals << val
    end
    vals.should == %w[one two three]
    hash.should == { 2 => 'two', 3 => 'three' }
    hash.size.should == 2
  end

  it 'a future key can be deleted during iteration' do
    hash = { 1 => 'one', 2 => 'two', 3 => 'three' }
    vals = []
    hash.each do |key, val|
      hash.delete(2)
      vals << val
    end
    vals.should == %w[one three]
    hash.should == { 1 => 'one', 3 => 'three' }
    hash.size.should == 2
  end

  it 'all keys can be deleted during iteration' do
    hash = { 1 => 'one', 2 => 'two', 3 => 'three' }
    vals = []
    hash.each do |key, val|
      hash.delete(1)
      hash.delete(2)
      hash.delete(3)
      vals << val
    end
    vals.should == ['one']
    hash.should == {}
    hash.size.should == 0
  end

  it 'can return its keys' do
    hash = { 1 => 'one', 2 => 'two', 3 => 'three' }
    hash.keys.should == [1, 2, 3]
  end

  it 'can return its values' do
    hash = { 1 => 'one', 2 => 'two', 3 => 'three' }
    hash.values.should == %w[one two three]
  end

  describe '#key?' do
    it 'returns true if the key exists' do
      h = { 1 => 1 }
      h.key?(1).should == true
    end

    it 'returns false if the key does not exist' do
      h = { 1 => 1 }
      h.key?(2).should == false
    end

    it 'returns false if the key does not exist, even if there is a default hash value' do
      h = Hash.new(2)
      h[1].should == 2
      h.key?(1).should == false
      h = Hash.new { 2 }
      h[1].should == 2
      h.key?(1).should == false
    end
  end

  it 'handles two keys with the same hash' do
    class MyDumbKey
      def hash
        111
      end
    end

    a = MyDumbKey.new
    b = MyDumbKey.new
    h = { a => 1, b => 2 }
    h[a].should_not == h[b]
  end

  it "handles two identical keys when #eql? isn't reflexive" do
    x = mock('x')
    x.should_receive(:hash).at_least(1).and_return(42)
    x.stub!(:eql?).and_return(false)
    hash = { x => 1 }
    hash[x] = 2
    hash.should == { x => 2 }
  end

  describe 'merge/update' do
    it 'creates a new copy of the hash with the given hashes merged in' do
      h = { 0 => 0, 1 => 1 }
      h.merge({ 2 => 2 }, { 3 => 3, 1 => 10 }).should == { 0 => 0, 1 => 10, 2 => 2, 3 => 3 }
      h.update({ 2 => 2 }, { 3 => 3, 1 => 10 }).should == { 0 => 0, 1 => 10, 2 => 2, 3 => 3 }
    end
  end

  describe 'merge!' do
    it 'merges given hashes into self, changing self in-place' do
      h = { 0 => 0, 1 => 1 }
      h.merge!({ 2 => 2 }, { 3 => 3, 1 => 10 })
      h.should == { 0 => 0, 1 => 10, 2 => 2, 3 => 3 }
    end
  end

  describe '#values_at' do
    it 'return default value of key not in hash' do
      h = { a: 1, b: 2 }
      h.default = 3
      h.values_at(:a, :b, :c).should == [1, 2, 3]
    end
  end

  describe '#value?' do
    it 'work with falsey values' do
      h = { nil => :a }
      h.value?(:a).should == true
    end
  end

  describe '#each' do
    it 'can return without leaving the hash in a bad state' do
      def first_key(h)
        h.each { |k, v| return k }
      end
      hash = { a: 1 }
      first_key(hash)
      -> { hash[:b] = 2 }.should_not raise_error
    end

    it 'can raise an exception without leaving the hash in a bad state' do
      class IgnoreThisError < StandardError
      end
      def first_key(h)
        h.each { |k, v| raise IgnoreThisError }
      end
      hash = { a: 1 }
      begin
        first_key(hash)
      rescue StandardError
      end
      -> { hash[:b] = 2 }.should_not raise_error
    end
  end
end
