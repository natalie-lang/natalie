require_relative '../spec_helper'

describe 'Kernel' do
  describe '#enum_for' do
    it 'returns a new Enumerator created by calling the given method' do
      a = [1, 2, 3]
      enum = a.enum_for(:each)
      enum.should be_an_instance_of(Enumerator)
      enum.to_a.should == [1, 2, 3]
    end
  end

  describe '#is_a?' do
    it 'returns false with a module and itself' do
      Enumerable.is_a?(Enumerable).should == false
    end

    it 'returns true with the Module object and itself' do
      Module.is_a?(Module).should == true
    end

    it 'returns true with a module and the Module object' do
      Enumerable.is_a?(Module).should == true
    end

    it 'returns true with the Class object and the Module object' do
      Class.is_a?(Module).should == true
    end

    it 'returns true with the Module object and the Class object' do
      Module.is_a?(Class).should == true
    end
  end

  describe '#instance_of?' do
    it "can be bound to BasicObject" do
      klass = Class.new(BasicObject) do
        define_method :instance_of?, Kernel.instance_method(:instance_of?)
      end
      klass.new.instance_of?(klass).should == true
    end
  end

  describe '#sleep' do
    it 'sleeps the number of seconds' do
      t_start = Process.clock_gettime(Process::CLOCK_MONOTONIC_RAW)
      sleep 1
      t_end = Process.clock_gettime(Process::CLOCK_MONOTONIC_RAW)
      (t_end - t_start).should >= 0.9
      (t_end - t_start).should <= 1.2

      t_start = Process.clock_gettime(Process::CLOCK_MONOTONIC_RAW)
      sleep 0.1
      t_end = Process.clock_gettime(Process::CLOCK_MONOTONIC_RAW)
      (t_end - t_start).should >= 0.01
      (t_end - t_start).should <= 0.3
    end
  end

  describe '#redo' do
    it 'works with loops' do
      a = []
      loop do
        a << 1
        redo if a.size < 2 # restart the loop
        a << 2
        break if a.size >= 3
      end
      a.should == [1, 1, 2]
    end

    it 'works with each' do
      a = []
      [1, 2, 3].each do |i|
        a << i
        redo if a.size < 2 # restart the loop
      end
      a.should == [1, 1, 2, 3]
    end

    it 'works within a File block' do
      a = []
      File.open('test/support/file.txt') do
        a << 1
        redo if a.size < 2
      end
      a.should == [1, 1]
    end

    it 'works in a Proc' do
      a = []
      p =
        Proc.new do
          a << 1
          redo if a.size < 2
        end
      p.call
      a.should == [1, 1]
    end
  end

  describe '#p' do
    it 'returns the argument' do
      result = nil
      (-> { result = p([1, 2, 3]) }).should output("[1, 2, 3]\n")
      result.should == [1, 2, 3]
    end
  end

  describe '#Complex' do
    it 'raises error with extra keywords' do
      -> { Complex(1, foo: 2, bar: 3) }.should raise_error(ArgumentError, 'unknown keywords: :foo, :bar')
    end
  end

  describe '#Float' do
    it 'raises error with extra keywords' do
      -> { Float(1, foo: 2, bar: 3) }.should raise_error(ArgumentError, 'unknown keywords: :foo, :bar')
    end
  end

  describe '#Rational' do
    it 'raises error with extra keywords' do
      -> { Rational(1, foo: 2, bar: 3) }.should raise_error(ArgumentError, 'unknown keywords: :foo, :bar')
    end
  end
end
