require_relative '../../spec_helper'

describe "MatchData#values_at" do
  # Should be synchronized with core/array/values_at_spec.rb and core/struct/values_at_spec.rb
  #
  # /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").to_a # => ["HX1138", "H", "X", "113", "8"]

  context "when passed a list of Integers" do
    it "returns an array containing each value given by one of integers" do
      /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at(0, 2, -2).should == ["HX1138", "X", "113"]
    end

    it "returns nil value for any integer that is out of range" do
      /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at(5).should == [nil]
      /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at(-6).should == [nil]
    end
  end

  context "when passed an integer Range" do
    it "returns an array containing each value given by the elements of the range" do
      NATFIXME 'returns an array containing each value given by the elements of the range', exception: SpecFailedException do
        /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at(0..2).should == ["HX1138", "H", "X"]
      end
    end

    it "fills with nil values for range elements larger than the captured values number" do
      NATFIXME 'fills with nil values for range elements larger than the captured values number', exception: SpecFailedException do
        /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at(0..5).should == ["HX1138", "H", "X", "113", "8", nil]
      end
    end

    it "raises RangeError if any element of the range is negative and out of range" do
      NATFIXME 'raises RangeError if any element of the range is negative and out of range', exception: SpecFailedException do
        -> { /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at(-6..3) }.should raise_error(RangeError, "-6..3 out of range")
      end
    end

    it "supports endless Range" do
      NATFIXME 'supports endless Range', exception: SpecFailedException do
        /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at(0..).should == ["HX1138", "H", "X", "113", "8"]
      end
    end

    it "supports beginningless Range" do
      NATFIXME 'supports beginningless Range', exception: SpecFailedException do
        /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at(..2).should == ["HX1138", "H", "X"]
      end
    end

    it "returns an empty Array when Range is empty" do
      /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at(2..0).should == []
    end
  end

  context "when passed names" do
    it 'slices captures with the given names' do
      NATFIXME 'slices captures with the given names', exception: SpecFailedException do
        /(?<a>.)(?<b>.)(?<c>.)/.match('012').values_at(:c, :a).should == ['2', '0']
      end
    end

    it 'slices captures with the given String names' do
      NATFIXME 'slices captures with the given String names', exception: SpecFailedException do
        /(?<a>.)(?<b>.)(?<c>.)/.match('012').values_at('c', 'a').should == ['2', '0']
      end
    end
  end

  it "supports multiple integer Ranges" do
    NATFIXME 'supports multiple integer Ranges', exception: SpecFailedException do
      /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at(1..2, 2..3).should == ["H", "X", "X", "113"]
    end
  end

  it "supports mixing integer Ranges and Integers" do
    NATFIXME 'supports mixing integer Ranges and Integers', exception: SpecFailedException do
      /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at(1..2, 4).should == ["H", "X", "8"]
    end
  end

  it 'supports mixing of names and indices' do
    NATFIXME 'supports mixing of names and indices', exception: SpecFailedException do
      /\A(?<a>.)(?<b>.)\z/.match('01').values_at(0, 1, 2, :a, :b).should == ['01', '0', '1', '0', '1']
    end
  end

  it "returns a new empty Array if no arguments given" do
    /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at().should == []
  end

  it "fails when passed arguments of unsupported types" do
    NATFIXME 'fails when passed arguments of unsupported types', exception: SpecFailedException do
      -> {
        /(.)(.)(\d+)(\d)/.match("THX1138: The Movie").values_at(Object.new)
      }.should raise_error(TypeError, "no implicit conversion of Object into Integer")
    end
  end
end
