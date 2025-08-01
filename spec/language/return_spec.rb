require_relative '../spec_helper'
require_relative 'fixtures/return'

describe "The return keyword" do
  it "returns any object directly" do
    def r; return 1; end
    r().should == 1
  end

  it "returns an single element array directly" do
    def r; return [1]; end
    r().should == [1]
  end

  it "returns an multi element array directly" do
    def r; return [1,2]; end
    r().should == [1,2]
  end

  it "returns nil by default" do
    def r; return; end
    r().should be_nil
  end

  describe "in a Thread" do
    it "raises a LocalJumpError if used to exit a thread" do
      t = Thread.new {
        begin
          return
        rescue LocalJumpError => e
          e
        end
      }
      t.value.should be_an_instance_of(LocalJumpError)
    end
  end

  describe "when passed a splat" do
    it "returns [] when the ary is empty" do
      def r; ary = []; return *ary; end
      r.should == []
    end

    it "returns the array when the array is size of 1" do
      def r; ary = [1]; return *ary; end
      r.should == [1]
    end

    it "returns the whole array when size is greater than 1" do
      def r; ary = [1,2]; return *ary; end
      r.should == [1,2]

      def r; ary = [1,2,3]; return *ary; end
      r.should == [1,2,3]
    end

    it "returns an array when used as a splat" do
      def r; value = 1; return *value; end
      NATFIXME 'it returns an array when used as a splat', exception: SpecFailedException do
        r.should == [1]
      end
    end

    it "calls 'to_a' on the splatted value first" do
      def r
        obj = Object.new
        def obj.to_a
          [1,2]
        end

        return *obj
      end

      NATFIXME "it calls 'to_a' on the splatted value first", exception: SpecFailedException do
        r().should == [1,2]
      end
    end
  end

  describe "within a begin" do
    before :each do
      ScratchPad.record []
    end

    it "executes ensure before returning" do
      def f()
        begin
          ScratchPad << :begin
          return :begin
          ScratchPad << :after_begin
        ensure
          ScratchPad << :ensure
        end
        ScratchPad << :function
      end
      f().should == :begin
      NATFIXME 'it executes ensure before returning', exception: SpecFailedException do
        ScratchPad.recorded.should == [:begin, :ensure]
      end
    end

    it "returns last value returned in ensure" do
      def f()
        begin
          ScratchPad << :begin
          return :begin
          ScratchPad << :after_begin
        ensure
          ScratchPad << :ensure
          return :ensure
          ScratchPad << :after_ensure
        end
        ScratchPad << :function
      end
      NATFIXME 'it returns last value returned in ensure', exception: SpecFailedException do
        f().should == :ensure
        ScratchPad.recorded.should == [:begin, :ensure]
      end
    end

    it "executes nested ensures before returning" do
      def f()
        begin
          begin
            ScratchPad << :inner_begin
            return :inner_begin
            ScratchPad << :after_inner_begin
          ensure
            ScratchPad << :inner_ensure
          end
          ScratchPad << :outer_begin
          return :outer_begin
          ScratchPad << :after_outer_begin
        ensure
          ScratchPad << :outer_ensure
        end
        ScratchPad << :function
      end
      f().should == :inner_begin
      NATFIXME 'it executes nested ensures before returning', exception: SpecFailedException do
        ScratchPad.recorded.should == [:inner_begin, :inner_ensure, :outer_ensure]
      end
    end

    it "returns last value returned in nested ensures" do
      def f()
        begin
          begin
            ScratchPad << :inner_begin
            return :inner_begin
            ScratchPad << :after_inner_begin
          ensure
            ScratchPad << :inner_ensure
            return :inner_ensure
            ScratchPad << :after_inner_ensure
          end
          ScratchPad << :outer_begin
          return :outer_begin
          ScratchPad << :after_outer_begin
        ensure
          ScratchPad << :outer_ensure
          return :outer_ensure
          ScratchPad << :after_outer_ensure
        end
        ScratchPad << :function
      end
      NATFIXME 'it returns last value returned in nested ensures', exception: SpecFailedException do
        f().should == :outer_ensure
        ScratchPad.recorded.should == [:inner_begin, :inner_ensure, :outer_ensure]
      end
    end

    it "executes the ensure clause when begin/ensure are inside a lambda" do
      -> do
        begin
          return
        ensure
          ScratchPad.recorded << :ensure
        end
      end.call
      ScratchPad.recorded.should == [:ensure]
    end
  end

  describe "within a block" do
    before :each do
      ScratchPad.clear
    end

    it "causes lambda to return nil if invoked without any arguments" do
      -> { return; 456 }.call.should be_nil
    end

    it "causes lambda to return nil if invoked with an empty expression" do
      -> { return (); 456 }.call.should be_nil
    end

    it "causes lambda to return the value passed to return" do
      -> { return 123; 456 }.call.should == 123
    end

    it "causes the method that lexically encloses the block to return" do
      ReturnSpecs::Blocks.new.enclosing_method.should == :return_value
      ScratchPad.recorded.should == :before_return
    end

    it "returns from the lexically enclosing method even in case of chained calls" do
      ReturnSpecs::NestedCalls.new.enclosing_method.should == :return_value
      ScratchPad.recorded.should == :before_return
    end

    it "returns from the lexically enclosing method even in case of chained calls(in yield)" do
      ReturnSpecs::NestedBlocks.new.enclosing_method.should == :return_value
      ScratchPad.recorded.should == :before_return
    end

    it "causes the method to return even when the immediate parent has already returned" do
      ReturnSpecs::SavedInnerBlock.new.start.should == :return_value
      ScratchPad.recorded.should == :before_return
    end

    # jruby/jruby#3143
    describe "downstream from a lambda" do
      it "returns to its own return-capturing lexical enclosure" do
        def a
          ->{ yield }.call
          return 2
        end
        def b
          a { return 1 }
        end

        b.should == 1
      end
    end

  end

  describe "within two blocks" do
    it "causes the method that lexically encloses the block to return" do
      def f
        1.times { 1.times {return true}; false}; false
      end
      f.should be_true
    end
  end

  describe "within define_method" do
    # NATFIXME: unexpected return (LocalJumpError)
    xit "goes through the method via a closure" do
      ReturnSpecs::ThroughDefineMethod.new.outer.should == :good
    end

    it "stops at the method when the return is used directly" do
      ReturnSpecs::DefineMethod.new.outer.should == :good
    end
  end

  describe "invoked with a method call without parentheses with a block" do
    it "returns the value returned from the method call" do
      ReturnSpecs::MethodWithBlock.new.method1.should == 5
      ReturnSpecs::MethodWithBlock.new.method2.should == [0, 1, 2]
    end
  end

  describe "at top level" do
    before :each do
      @filename = tmp("top_return.rb")
      ScratchPad.record []
    end

    after do
      rm_r @filename
    end

    it "stops file execution" do
      ruby_exe(<<-END_OF_CODE).should == "before return\n"
        puts "before return"
        return

        puts "after return"
      END_OF_CODE

      $?.exitstatus.should == 0
    end

    describe "within if" do
      # NATFIXME: Compile time error, load with non-static value
      xit "is allowed" do
        File.write(@filename, <<-END_OF_CODE)
          ScratchPad << "before if"
          if true
            return
          end

          ScratchPad << "after if"
        END_OF_CODE

        #load @filename
        ScratchPad.recorded.should == ["before if"]
      end
    end

    describe "within while loop" do
      # NATFIXME: Compile time error, load with non-static value
      xit "is allowed" do
        File.write(@filename, <<-END_OF_CODE)
          ScratchPad << "before while"
          while true
            return
          end

          ScratchPad << "after while"
        END_OF_CODE

        #load @filename
        ScratchPad.recorded.should == ["before while"]
      end
    end

    describe "within a begin" do
      # NATFIXME: Compile time error, load with non-static value
      xit "is allowed in begin block" do
        File.write(@filename, <<-END_OF_CODE)
          ScratchPad << "before begin"
          begin
            return
          end

          ScratchPad << "after begin"
        END_OF_CODE

        #load @filename
        ScratchPad.recorded.should == ["before begin"]
      end

      # NATFIXME: Compile time error, load with non-static value
      xit "is allowed in ensure block" do
        File.write(@filename, <<-END_OF_CODE)
          ScratchPad << "before begin"
          begin
          ensure
            return
          end

          ScratchPad << "after begin"
        END_OF_CODE

        #load @filename
        ScratchPad.recorded.should == ["before begin"]
      end

      # NATFIXME: Compile time error, load with non-static value
      xit "is allowed in rescue block" do
        File.write(@filename, <<-END_OF_CODE)
          ScratchPad << "before begin"
          begin
            raise
          rescue RuntimeError
            return
          end

          ScratchPad << "after begin"
        END_OF_CODE

        #load @filename
        ScratchPad.recorded.should == ["before begin"]
      end

      it "fires ensure block before returning" do
        NATFIXME 'it fires ensure block before returning', exception: SpecFailedException do
          ruby_exe(<<-END_OF_CODE).should == "within ensure\n"
            begin
              return
            ensure
              puts "within ensure"
            end

            puts "after begin"
          END_OF_CODE
        end
      end

      # NATFIXME: Compile time error, load with non-static value
      xit "fires ensure block before returning while loads file" do
        File.write(@filename, <<-END_OF_CODE)
          ScratchPad << "before begin"
          begin
            return
          ensure
            ScratchPad << "within ensure"
          end

          ScratchPad << "after begin"
        END_OF_CODE

        #load @filename
        ScratchPad.recorded.should == ["before begin", "within ensure"]
      end

      # NATFIXME: Compile time error, load with non-static value
      xit "swallows exception if returns in ensure block" do
        File.write(@filename, <<-END_OF_CODE)
          begin
            raise
          ensure
            ScratchPad << "before return"
            return
          end
        END_OF_CODE

        #load @filename
        ScratchPad.recorded.should == ["before return"]
      end
    end

    describe "within a block" do
      # NATFIXME: Compile time error, load with non-static value
      xit "is allowed" do
        File.write(@filename, <<-END_OF_CODE)
          ScratchPad << "before call"
          proc { return }.call

          ScratchPad << "after call"
        END_OF_CODE

        #load @filename
        ScratchPad.recorded.should == ["before call"]
      end
    end

    describe "within a class" do
      # NATFIXME: Compile time error, load with non-static value
      xit "raises a SyntaxError" do
        File.write(@filename, <<-END_OF_CODE)
          class ReturnSpecs::A
            ScratchPad << "before return"
            return

            ScratchPad << "after return"
          end
        END_OF_CODE

        #-> { load @filename }.should raise_error(SyntaxError)
      end
    end

    describe "within a block within a class" do
      # NATFIXME: Compile time error, load with non-static value
      xit "is not allowed" do
        File.write(@filename, <<-END_OF_CODE)
          class ReturnSpecs::A
            ScratchPad << "before return"
            1.times { return }
            ScratchPad << "after return"
          end
        END_OF_CODE

        #-> { load @filename }.should raise_error(LocalJumpError)
      end
    end

    describe "within BEGIN" do
      # NATFIXME: Compile time error, load with non-static value
      xit "is allowed" do
        File.write(@filename, <<-END_OF_CODE)
          BEGIN {
            ScratchPad << "before call"
            return
            ScratchPad << "after call"
          }
        END_OF_CODE

        #load @filename
        ScratchPad.recorded.should == ["before call"]
      end
    end

    describe "file loading" do
      # NATFIXME: Compile time error, load with non-static value
      xit "stops file loading and execution" do
        File.write(@filename, <<-END_OF_CODE)
          ScratchPad << "before return"
          return
          ScratchPad << "after return"
        END_OF_CODE

        #load @filename
        ScratchPad.recorded.should == ["before return"]
      end
    end

    describe "file requiring" do
      # NATFIXME: Compile time error, require with non-static value
      xit "stops file loading and execution" do
        File.write(@filename, <<-END_OF_CODE)
          ScratchPad << "before return"
          return
          ScratchPad << "after return"
        END_OF_CODE

        #require @filename
        ScratchPad.recorded.should == ["before return"]
      end
    end

    describe "return with argument" do
      it "warns but does not affect exit status" do
        err = ruby_exe(<<-END_OF_CODE, args: "2>&1")
          return 10
        END_OF_CODE
        $?.exitstatus.should == 0

        NATFIXME 'it warns', exception: SpecFailedException do
          err.should =~ /warning: argument of top-level return is ignored/
        end
      end
    end
  end
end
