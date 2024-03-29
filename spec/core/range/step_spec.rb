require_relative '../../spec_helper'

describe "Range#step" do
  before :each do
    ScratchPad.record []
  end

  it "returns self" do
    r = 1..2
    r.step { }.should equal(r)
  end

  it "raises TypeError if step" do
    obj = mock("mock")
    -> { (1..10).step(obj) { } }.should raise_error(TypeError)
  end

  it "calls #to_int to coerce step to an Integer" do
    obj = mock("Range#step")
    obj.should_receive(:to_int).and_return(1)

    (1..2).step(obj) { |x| ScratchPad << x }
    ScratchPad.recorded.should eql([1, 2])
  end

  it "raises a TypeError if step does not respond to #to_int" do
    obj = mock("Range#step non-integer")

    -> { (1..2).step(obj) { } }.should raise_error(TypeError)
  end

  it "raises a TypeError if #to_int does not return an Integer" do
    obj = mock("Range#step non-integer")
    obj.should_receive(:to_int).and_return("1")

    -> { (1..2).step(obj) { } }.should raise_error(TypeError)
  end

  it "coerces the argument to integer by invoking to_int" do
    (obj = mock("2")).should_receive(:to_int).and_return(2)
    res = []
    (1..10).step(obj) {|x| res << x}
    res.should == [1, 3, 5, 7, 9]
  end

  it "raises a TypeError if the first element does not respond to #succ" do
    obj = mock("Range#step non-comparable")
    obj.should_receive(:<=>).with(obj).and_return(1)

    -> { (obj..obj).step { |x| x } }.should raise_error(TypeError)
  end

  it "raises an ArgumentError if step is 0" do
    -> { (-1..1).step(0) { |x| x } }.should raise_error(ArgumentError)
  end

  it "raises an ArgumentError if step is 0.0" do
    -> { (-1..1).step(0.0) { |x| x } }.should raise_error(ArgumentError)
  end

  it "raises an ArgumentError if step is negative" do
    -> { (-1..1).step(-2) { |x| x } }.should raise_error(ArgumentError)
  end

  describe "with inclusive end" do
    describe "and Integer values" do
      it "yields Integer values incremented by 1 and less than or equal to end when not passed a step" do
        (-2..2).step { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-2, -1, 0, 1, 2])
      end

      it "yields Integer values incremented by an Integer step" do
        (-5..5).step(2) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-5, -3, -1, 1, 3, 5])
      end

      it "yields Float values incremented by a Float step" do
        (-2..2).step(1.5) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-2.0, -0.5, 1.0])
      end
    end

    describe "and Float values" do
      it "yields Float values incremented by 1 and less than or equal to end when not passed a step" do
        (-2.0..2.0).step { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-2.0, -1.0, 0.0, 1.0, 2.0])
      end

      it "yields Float values incremented by an Integer step" do
        (-5.0..5.0).step(2) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-5.0, -3.0, -1.0, 1.0, 3.0, 5.0])
      end

      it "yields Float values incremented by a Float step" do
        (-1.0..1.0).step(0.5) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-1.0, -0.5, 0.0, 0.5, 1.0])
      end

      it "returns Float values of 'step * n + begin <= end'" do
        (1.0..6.4).step(1.8) { |x| ScratchPad << x }
        (1.0..12.7).step(1.3) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([1.0, 2.8, 4.6, 6.4, 1.0, 2.3, 3.6,
                                       4.9, 6.2, 7.5, 8.8, 10.1, 11.4, 12.7])
      end

      it "handles infinite values at either end" do
        (-Float::INFINITY..0.0).step(2) { |x| ScratchPad << x; break if ScratchPad.recorded.size == 3 }
        ScratchPad.recorded.should eql([-Float::INFINITY, -Float::INFINITY, -Float::INFINITY])

        ScratchPad.record []
        (0.0..Float::INFINITY).step(2) { |x| ScratchPad << x; break if ScratchPad.recorded.size == 3 }
        ScratchPad.recorded.should eql([0.0, 2.0, 4.0])
      end
    end

    describe "and Integer, Float values" do
      it "yields Float values incremented by 1 and less than or equal to end when not passed a step" do
        (-2..2.0).step { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-2.0, -1.0, 0.0, 1.0, 2.0])
      end

      it "yields Float values incremented by an Integer step" do
        (-5..5.0).step(2) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-5.0, -3.0, -1.0, 1.0, 3.0, 5.0])
      end

      it "yields Float values incremented by a Float step" do
        (-1..1.0).step(0.5) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-1.0, -0.5, 0.0, 0.5, 1.0])
      end
    end

    describe "and Float, Integer values" do
      it "yields Float values incremented by 1 and less than or equal to end when not passed a step" do
        (-2.0..2).step { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-2.0, -1.0, 0.0, 1.0, 2.0])
      end

      it "yields Float values incremented by an Integer step" do
        (-5.0..5).step(2) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-5.0, -3.0, -1.0, 1.0, 3.0, 5.0])
      end

      it "yields Float values incremented by a Float step" do
        (-1.0..1).step(0.5) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-1.0, -0.5, 0.0, 0.5, 1.0])
      end
    end

    describe "and String values" do
      it "yields String values incremented by #succ and less than or equal to end when not passed a step" do
        ("A".."E").step { |x| ScratchPad << x }
        ScratchPad.recorded.should == ["A", "B", "C", "D", "E"]
      end

      it "yields String values incremented by #succ called Integer step times" do
        ("A".."G").step(2) { |x| ScratchPad << x }
        ScratchPad.recorded.should == ["A", "C", "E", "G"]
      end

      it "raises a TypeError when passed a Float step" do
        -> { ("A".."G").step(2.0) { } }.should raise_error(TypeError)
      end

      it "calls #succ on begin and each element returned by #succ" do
        obj = mock("Range#step String start")
        obj.should_receive(:<=>).exactly(3).times.and_return(-1, -1, -1, 0)
        obj.should_receive(:succ).exactly(2).times.and_return(obj)

        (obj..obj).step { |x| ScratchPad << x }
        ScratchPad.recorded.should == [obj, obj, obj]
      end
    end
  end

  describe "with exclusive end" do
    describe "and Integer values" do
      it "yields Integer values incremented by 1 and less than end when not passed a step" do
        (-2...2).step { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-2, -1, 0, 1])
      end

      it "yields Integer values incremented by an Integer step" do
        (-5...5).step(2) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-5, -3, -1, 1, 3])
      end

      it "yields Float values incremented by a Float step" do
        (-2...2).step(1.5) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-2.0, -0.5, 1.0])
      end
    end

    describe "and Float values" do
      it "yields Float values incremented by 1 and less than end when not passed a step" do
        (-2.0...2.0).step { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-2.0, -1.0, 0.0, 1.0])
      end

      it "yields Float values incremented by an Integer step" do
        (-5.0...5.0).step(2) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-5.0, -3.0, -1.0, 1.0, 3.0])
      end

      it "yields Float values incremented by a Float step" do
        (-1.0...1.0).step(0.5) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-1.0, -0.5, 0.0, 0.5])
      end

      it "returns Float values of 'step * n + begin < end'" do
        (1.0...6.4).step(1.8) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([1.0, 2.8, 4.6])
      end

      ruby_version_is '3.1' do
        it "correctly handles values near the upper limit" do # https://bugs.ruby-lang.org/issues/16612
          (1.0...55.6).step(18.2) { |x| ScratchPad << x }
          ScratchPad.recorded.should eql([1.0, 19.2, 37.4, 55.599999999999994])

          (1.0...55.6).step(18.2).size.should == 4
        end
      end

      it "handles infinite values at either end" do
        (-Float::INFINITY...0.0).step(2) { |x| ScratchPad << x; break if ScratchPad.recorded.size == 3 }
        ScratchPad.recorded.should eql([-Float::INFINITY, -Float::INFINITY, -Float::INFINITY])

        ScratchPad.record []
        (0.0...Float::INFINITY).step(2) { |x| ScratchPad << x; break if ScratchPad.recorded.size == 3 }
        ScratchPad.recorded.should eql([0.0, 2.0, 4.0])
      end
    end

    describe "and Integer, Float values" do
      it "yields Float values incremented by 1 and less than end when not passed a step" do
        (-2...2.0).step { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-2.0, -1.0, 0.0, 1.0])
      end

      it "yields Float values incremented by an Integer step" do
        (-5...5.0).step(2) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-5.0, -3.0, -1.0, 1.0, 3.0])
      end

      it "yields an Float and then Float values incremented by a Float step" do
        (-1...1.0).step(0.5) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-1.0, -0.5, 0.0, 0.5])
      end
    end

    describe "and Float, Integer values" do
      it "yields Float values incremented by 1 and less than end when not passed a step" do
        (-2.0...2).step { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-2.0, -1.0, 0.0, 1.0])
      end

      it "yields Float values incremented by an Integer step" do
        (-5.0...5).step(2) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-5.0, -3.0, -1.0, 1.0, 3.0])
      end

      it "yields Float values incremented by a Float step" do
        (-1.0...1).step(0.5) { |x| ScratchPad << x }
        ScratchPad.recorded.should eql([-1.0, -0.5, 0.0, 0.5])
      end
    end

    describe "and String values" do
      it "yields String values incremented by #succ and less than or equal to end when not passed a step" do
        ("A"..."E").step { |x| ScratchPad << x }
        ScratchPad.recorded.should == ["A", "B", "C", "D"]
      end

      it "yields String values incremented by #succ called Integer step times" do
        ("A"..."G").step(2) { |x| ScratchPad << x }
        ScratchPad.recorded.should == ["A", "C", "E"]
      end

      it "raises a TypeError when passed a Float step" do
        -> { ("A"..."G").step(2.0) { } }.should raise_error(TypeError)
      end
    end
  end

  describe "with an endless range" do
    describe "and Integer values" do
      it "yield Integer values incremented by 1 when not passed a step" do
        eval("(-2..)").step { |x| break if x > 2; ScratchPad << x }
        ScratchPad.recorded.should eql([-2, -1, 0, 1, 2])

        ScratchPad.record []
        eval("(-2...)").step { |x| break if x > 2; ScratchPad << x }
        ScratchPad.recorded.should eql([-2, -1, 0, 1, 2])
      end

      it "yields Integer values incremented by an Integer step" do
        eval("(-5..)").step(2) { |x| break if x > 3; ScratchPad << x }
        ScratchPad.recorded.should eql([-5, -3, -1, 1, 3])

        ScratchPad.record []
        eval("(-5...)").step(2) { |x| break if x > 3; ScratchPad << x }
        ScratchPad.recorded.should eql([-5, -3, -1, 1, 3])
      end

      it "yields Float values incremented by a Float step" do
        eval("(-2..)").step(1.5) { |x| break if x > 1.0; ScratchPad << x }
        ScratchPad.recorded.should eql([-2.0, -0.5, 1.0])

        ScratchPad.record []
        eval("(-2..)").step(1.5) { |x| break if x > 1.0; ScratchPad << x }
        ScratchPad.recorded.should eql([-2.0, -0.5, 1.0])
      end
    end

    describe "and Float values" do
      it "yields Float values incremented by 1 and less than end when not passed a step" do
        eval("(-2.0..)").step { |x| break if x > 1.5; ScratchPad << x }
        ScratchPad.recorded.should eql([-2.0, -1.0, 0.0, 1.0])

        ScratchPad.record []
        eval("(-2.0...)").step { |x| break if x > 1.5; ScratchPad << x }
        ScratchPad.recorded.should eql([-2.0, -1.0, 0.0, 1.0])
      end

      it "yields Float values incremented by an Integer step" do
        eval("(-5.0..)").step(2) { |x| break if x > 3.5; ScratchPad << x }
        ScratchPad.recorded.should eql([-5.0, -3.0, -1.0, 1.0, 3.0])

        ScratchPad.record []
        eval("(-5.0...)").step(2) { |x| break if x > 3.5; ScratchPad << x }
        ScratchPad.recorded.should eql([-5.0, -3.0, -1.0, 1.0, 3.0])
      end

      it "yields Float values incremented by a Float step" do
        eval("(-1.0..)").step(0.5) { |x| break if x > 0.6; ScratchPad << x }
        ScratchPad.recorded.should eql([-1.0, -0.5, 0.0, 0.5])

        ScratchPad.record []
        eval("(-1.0...)").step(0.5) { |x| break if x > 0.6; ScratchPad << x }
        ScratchPad.recorded.should eql([-1.0, -0.5, 0.0, 0.5])
      end

      it "handles infinite values at the start" do
        eval("(-Float::INFINITY..)").step(2) { |x| ScratchPad << x; break if ScratchPad.recorded.size == 3 }
        ScratchPad.recorded.should eql([-Float::INFINITY, -Float::INFINITY, -Float::INFINITY])

        ScratchPad.record []
        eval("(-Float::INFINITY...)").step(2) { |x| ScratchPad << x; break if ScratchPad.recorded.size == 3 }
        ScratchPad.recorded.should eql([-Float::INFINITY, -Float::INFINITY, -Float::INFINITY])
      end
    end

    describe "and String values" do
      it "yields String values incremented by #succ and less than or equal to end when not passed a step" do
        eval("('A'..)").step { |x| break if x > "D"; ScratchPad << x }
        ScratchPad.recorded.should == ["A", "B", "C", "D"]

        ScratchPad.record []
        eval("('A'...)").step { |x| break if x > "D"; ScratchPad << x }
        ScratchPad.recorded.should == ["A", "B", "C", "D"]
      end

      it "yields String values incremented by #succ called Integer step times" do
        eval("('A'..)").step(2) { |x| break if x > "F"; ScratchPad << x }
        ScratchPad.recorded.should == ["A", "C", "E"]

        ScratchPad.record []
        eval("('A'...)").step(2) { |x| break if x > "F"; ScratchPad << x }
        ScratchPad.recorded.should == ["A", "C", "E"]
      end

      it "raises a TypeError when passed a Float step" do
        -> { eval("('A'..)").step(2.0) { } }.should raise_error(TypeError)
        -> { eval("('A'...)").step(2.0) { } }.should raise_error(TypeError)
      end
    end
  end

  describe "when no block is given" do
    it "raises an ArgumentError if step is 0" do
      -> { (-1..1).step(0) }.should raise_error(ArgumentError)
    end

    describe "returned Enumerator" do
      describe "size" do
        it "raises a TypeError if step does not respond to #to_int" do
          obj = mock("Range#step non-integer")
          -> { (1..2).step(obj) }.should raise_error(TypeError)
        end

        it "raises a TypeError if #to_int does not return an Integer" do
          obj = mock("Range#step non-integer")
          obj.should_receive(:to_int).and_return("1")
          -> { (1..2).step(obj) }.should raise_error(TypeError)
        end

        it "returns the ceil of range size divided by the number of steps" do
          (1..10).step(4).size.should == 3
          (1..10).step(3).size.should == 4
          (1..10).step(2).size.should == 5
          (1..10).step(1).size.should == 10
          (-5..5).step(2).size.should == 6
          (1...10).step(4).size.should == 3
          (1...10).step(3).size.should == 3
          (1...10).step(2).size.should == 5
          (1...10).step(1).size.should == 9
          (-5...5).step(2).size.should == 5
        end

        it "returns the ceil of range size divided by the number of steps even if step is negative" do
          (-1..1).step(-1).size.should == 0
          (1..-1).step(-1).size.should == 3
        end

        it "returns the correct number of steps when one of the arguments is a float" do
          (-1..1.0).step(0.5).size.should == 5
          (-1.0...1.0).step(0.5).size.should == 4
        end

        it "returns the range size when there's no step_size" do
          (-2..2).step.size.should == 5
          (-2.0..2.0).step.size.should == 5
          (-2..2.0).step.size.should == 5
          (-2.0..2).step.size.should == 5
          (1.0..6.4).step(1.8).size.should == 4
          (1.0..12.7).step(1.3).size.should == 10
          (-2...2).step.size.should == 4
          (-2.0...2.0).step.size.should == 4
          (-2...2.0).step.size.should == 4
          (-2.0...2).step.size.should == 4
          (1.0...6.4).step(1.8).size.should == 3
        end

        it "returns nil with begin and end are String" do
          ("A".."E").step(2).size.should == nil
          ("A"..."E").step(2).size.should == nil
          ("A".."E").step.size.should == nil
          ("A"..."E").step.size.should == nil
        end

        it "return nil and not raises a TypeError if the first element does not respond to #succ" do
          obj = mock("Range#step non-comparable")
          obj.should_receive(:<=>).with(obj).and_return(1)
          enum = (obj..obj).step
          -> { enum.size }.should_not raise_error
          enum.size.should == nil
        end
      end

      describe "type" do
        context "when both begin and end are numerics" do
          it "returns an instance of Enumerator::ArithmeticSequence" do
            (1..10).step.class.should == Enumerator::ArithmeticSequence
          end
        end

        context "when begin is not defined and end is numeric" do
          it "returns an instance of Enumerator::ArithmeticSequence" do
            (..10).step.class.should == Enumerator::ArithmeticSequence
          end
        end

        context "when range is endless" do
          it "returns an instance of Enumerator::ArithmeticSequence when begin is numeric" do
            (1..).step.class.should == Enumerator::ArithmeticSequence
          end

          it "returns an instance of Enumerator when begin is not numeric" do
            ("a"..).step.class.should == Enumerator
          end
        end

        context "when range is beginless and endless" do
          it "returns an instance of Enumerator" do
            Range.new(nil, nil).step.class.should == Enumerator
          end
        end

        context "when begin and end are not numerics" do
          it "returns an instance of Enumerator" do
            ("a".."z").step.class.should == Enumerator
          end
        end
      end
    end
  end
end
