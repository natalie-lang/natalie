require_relative '../spec_helper'

describe 'Time' do
  def time
    Time.utc(1985, 4, 12, 23, 20, 50)
  end

  describe '.at' do
    context 'with a Rational argument' do
      it 'returns a time' do
        t = Time.at(Rational(1_486_570_508_539_759, 1_000_000))
        t.should be_an_instance_of(Time)
        t.usec.should == 539_759
      end
    end

    context 'with a String argument' do
      it 'raises an error' do
        -> { Time.at('') }.should raise_error(TypeError)
      end
    end

    context 'with a nil argument' do
      it 'raises an error' do
        -> { Time.at(nil) }.should raise_error(TypeError)
      end
    end
  end

  describe '#asctime' do
    it 'returns a string' do
      time.asctime.should == 'Fri Apr 12 23:20:50 1985'
    end
  end

  describe '#hour' do
    it 'returns an integer' do
      time.hour.should == 23
    end
  end

  describe '#inspect' do
    it 'returns a string' do
      time.inspect.should == '1985-04-12 23:20:50 UTC'
    end

    context 'with subseconds' do
      it 'includes the subseconds without trailing zeroes' do
        time = Time.utc(1985, 4, 12, 23, 20, 50, 520_000)
        time.inspect.should == '1985-04-12 23:20:50.52 UTC'
      end
    end
  end

  describe '#mday' do
    it 'returns an integer' do
      time.mday.should == 12
    end
  end

  describe '#min' do
    it 'returns an integer' do
      time.min.should == 20
    end
  end

  describe '#month' do
    it 'returns an integer' do
      time.month.should == 4
    end
  end

  describe '#nsec' do
    it 'returns an integer' do
      time = Time.utc(1985, 4, 12, 23, 20, 50, 520_000)
      time.nsec.should == 520_000_000
    end
  end

  describe '#sec' do
    it 'returns an integer' do
      time.sec.should == 50
    end
  end

  describe '#strftime' do
    it 'returns a string' do
      time.strftime('%Y-%m-%d %H:%M:%S').should == '1985-04-12 23:20:50'
    end
  end

  describe '#subsec' do
    it 'returns a rational' do
      time = Time.utc(1985, 4, 12, 23, 20, 50, 520_000)
      time.subsec.should == Rational(520_000, 1_000_000)
    end

    context 'without subseconds' do
      it 'returns zero' do
        time.subsec.should == 0
      end
    end
  end

  describe '#to_f' do
    it 'returns a float' do
      time = Time.utc(1985, 4, 12, 23, 20, 50, 520_000)
      time.to_f.should == 482196050.52
    end
  end

  describe '#to_i' do
    it 'returns an integer' do
      time.to_i.should == 482196050
    end
  end

  describe '#to_s' do
    it 'returns a string' do
      time.to_s.should == '1985-04-12 23:20:50 UTC'
    end
  end

  describe '#usec' do
    it 'returns an integer' do
      time = Time.utc(1985, 4, 12, 23, 20, 50, 520_000)
      time.usec.should == 520_000
    end
  end

  describe '#wday' do
    it 'returns an integer' do
      time.wday.should == 5
    end
  end

  describe '#yday' do
    it 'returns an integer' do
      time.yday.should == 102
    end
  end

  describe '#year' do
    it 'returns an integer' do
      time.year.should == 1985
    end
  end

  describe '#utc?' do
    context 'with a utc time' do
      it 'returns true' do
        time.utc?.should == true
      end
    end

    context 'with a local time' do
      it 'returns false' do
        time = Time.local(1985)
        time.utc?.should == false
      end
    end
  end
end
