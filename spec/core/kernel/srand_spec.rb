require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "Kernel#srand" do
  before :each do
    @seed = srand
  end

  after :each do
    srand(@seed)
  end

  it "is a private method" do
    Kernel.should have_private_instance_method(:srand)
  end

  it "returns the previous seed value" do
    srand(10)
    srand(20).should == 10
  end

  it "returns the system-initialized seed value on the first call" do
    ruby_exe('print srand(10)', options: '--disable-gems').should =~ /\A\d+\z/
  end

  it "seeds the RNG correctly and repeatably" do
    srand(10)
    x = rand
    srand(10)
    rand.should == x
  end

  it "defaults number to a random value" do
    -> { srand }.should_not raise_error
    srand.should_not == 0
  end

  it "accepts and uses a seed of 0" do
    srand(0)
    srand.should == 0
  end

  it "accepts a negative seed" do
    srand(-17)
    srand.should == -17
  end

  it "accepts an Integer as a seed" do
    NATFIXME 'Support bigint value', exception: RangeError, message: "bignum too big to convert into 'long long int'" do
      srand(0x12345678901234567890)
      srand.should == 0x12345678901234567890
    end
  end

  it "calls #to_int on seed" do
    srand(3.8)
    srand.should == 3

    s = mock('seed')
    s.should_receive(:to_int).and_return 0
    srand(s)
  end

  it "raises a TypeError when passed nil" do
    -> { srand(nil) }.should raise_error(TypeError)
  end

  it "raises a TypeError when passed a String" do
    -> { srand("7") }.should raise_error(TypeError)
  end
end

describe "Kernel.srand" do
  it "needs to be reviewed for spec completeness"
end
