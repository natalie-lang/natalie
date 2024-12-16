require_relative '../spec_helper'
require_relative 'fixtures/delegation'

describe "delegation with def(...)" do
  it "delegates rest and kwargs" do
    a = Class.new(DelegationSpecs::Target) do
      def delegate(...)
        target(...)
      end
    end

    a.new.delegate(1, b: 2).should == [[1], {b: 2}]
  end

  it "delegates block" do
    a = Class.new(DelegationSpecs::Target) do
      def delegate_block(...)
        target_block(...)
      end
    end

    a.new.delegate_block(1, b: 2) { |x| x }.should == [{b: 2}, [1]]
  end

  it "parses as open endless Range when brackets are omitted" do
    a = Class.new(DelegationSpecs::Target) do
      suppress_warning do
        def delegate(...)
          target ...
        end
      end
    end

    a.new.delegate(1, b: 2).should == Range.new([[], {}], nil, true)
  end
end

describe "delegation with def(x, ...)" do
  it "delegates rest and kwargs" do
    a = Class.new(DelegationSpecs::Target) do
      def delegate(x, ...)
        target(...)
      end
    end

    NATFIXME 'Partial delegation', exception: SpecFailedException do
      a.new.delegate(0, 1, b: 2).should == [[1], {b: 2}]
    end
  end

  it "delegates block" do
    a = Class.new(DelegationSpecs::Target) do
      def delegate_block(x, ...)
        target_block(...)
      end
    end

    NATFIXME 'Partial delegation', exception: SpecFailedException do
      a.new.delegate_block(0, 1, b: 2) { |x| x }.should == [{b: 2}, [1]]
    end
  end
end

ruby_version_is "3.2" do
  describe "delegation with def(*)" do
    it "delegates rest" do
      a = Class.new(DelegationSpecs::Target) do
        def delegate(*)
          target(*)
        end
      end

      a.new.delegate(0, 1).should == [[0, 1], {}]
    end
  end
end

ruby_version_is "3.2" do
  describe "delegation with def(**)" do
    it "delegates kwargs" do
      a = Class.new(DelegationSpecs::Target) do
        def delegate(**)
          target(**)
        end
      end

      a.new.delegate(a: 1) { |x| x }.should == [[], {a: 1}]
    end
  end
end
