require_relative '../../spec_helper'
require_relative 'fixtures/classes'

describe "Hash#to_proc" do
  before :each do
    @key = Object.new
    @value = Object.new
    @hash = { @key => @value }
    @default = Object.new
    @unstored = Object.new
  end

  it "returns an instance of Proc" do
    @hash.to_proc.should be_an_instance_of Proc
  end

  describe "the returned proc" do
    before :each do
      @proc = @hash.to_proc
    end

    it "is a lambda" do
      @proc.should.lambda?
    end

    it "has an arity of 1" do
      @proc.arity.should == 1
    end

    it "raises ArgumentError if not passed exactly one argument" do
      -> {
        @proc.call
      }.should raise_error(ArgumentError)

      -> {
        @proc.call 1, 2
      }.should raise_error(ArgumentError)
    end

    context "with a stored key" do
      it "returns the paired value" do
        @proc.call(@key).should equal(@value)
      end
    end

    context "passed as a block" do
      it "retrieves the hash's values" do
        [@key].map(&@proc)[0].should equal(@value)
      end

      context "to instance_exec" do
        # NATFIXME: I think this spec might even fail because we are using `self` in the returned proc
        it "always retrieves the original hash's values" do
          hash = {foo: 1, bar: 2}
          proc = hash.to_proc

          NATFIXME 'implement instance_exec', exception: NoMethodError, message: "undefined method `instance_exec'" do
            hash.instance_exec(:foo, &proc).should == 1

            hash2 = {quux: 1}
            hash2.instance_exec(:foo, &proc).should == 1
          end
        end
      end
    end

    context "with no stored key" do
      it "returns nil" do
        @proc.call(@unstored).should be_nil
      end

      context "when the hash has a default value" do
        before :each do
          @hash.default = @default
        end

        it "returns the default value" do
          @proc.call(@unstored).should equal(@default)
        end
      end

      context "when the hash has a default proc" do
        it "returns an evaluated value from the default proc" do
          @hash.default_proc = -> hash, called_with { [hash.keys, called_with] }
          @proc.call(@unstored).should == [[@key], @unstored]
        end
      end
    end

    it "raises an ArgumentError when calling #call on the Proc with no arguments" do
      -> { @hash.to_proc.call }.should raise_error(ArgumentError)
    end
  end
end
