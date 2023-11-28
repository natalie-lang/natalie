# encoding: utf-8

require_relative '../../spec_helper'
require_relative '../../fixtures/constants'
require_relative 'fixtures/constant_unicode'

describe "Module#const_defined?" do
  it "returns true if the given Symbol names a constant defined in the receiver" do
    ConstantSpecs.const_defined?(:CS_CONST2).should == true
    ConstantSpecs.const_defined?(:ModuleA).should == true
    ConstantSpecs.const_defined?(:ClassA).should == true
    ConstantSpecs::ContainerA.const_defined?(:ChildA).should == true
  end

  it "returns true if the constant is defined in the receiver's superclass" do
    # CS_CONST4 is defined in the superclass of ChildA
    ConstantSpecs::ContainerA::ChildA.const_defined?(:CS_CONST4).should be_true
  end

  it "returns true if the constant is defined in a mixed-in module of the receiver's parent" do
    # CS_CONST10 is defined in a module included by ChildA
    ConstantSpecs::ContainerA::ChildA.const_defined?(:CS_CONST10).should be_true
  end

  it "returns true if the constant is defined in a mixed-in module (with prepends) of the receiver" do
    # CS_CONST11 is defined in the module included by ContainerPrepend
    ConstantSpecs::ContainerPrepend.const_defined?(:CS_CONST11).should be_true
  end

  it "returns true if the constant is defined in Object and the receiver is a module" do
    # CS_CONST1 is defined in Object
    ConstantSpecs::ModuleA.const_defined?(:CS_CONST1).should be_true
  end

  it "returns true if the constant is defined in Object and the receiver is a class that has Object among its ancestors" do
    # CS_CONST1 is defined in Object
    ConstantSpecs::ContainerA::ChildA.const_defined?(:CS_CONST1).should be_true
  end

  it "returns false if the constant is defined in the receiver's superclass and the inherit flag is false" do
    NATFIXME 'Support inherited argument', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
      ConstantSpecs::ContainerA::ChildA.const_defined?(:CS_CONST4, false).should be_false
    end
  end

  it "returns true if the constant is defined in the receiver's superclass and the inherit flag is true" do
    NATFIXME 'Support inherited argument', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
      ConstantSpecs::ContainerA::ChildA.const_defined?(:CS_CONST4, true).should be_true
    end
  end

  it "coerces the inherit flag to a boolean" do
    NATFIXME 'Support inherited argument', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
      ConstantSpecs::ContainerA::ChildA.const_defined?(:CS_CONST4, nil).should be_false
      ConstantSpecs::ContainerA::ChildA.const_defined?(:CS_CONST4, :true).should be_true
    end
  end

  it "returns true if the given String names a constant defined in the receiver" do
    ConstantSpecs.const_defined?("CS_CONST2").should == true
    ConstantSpecs.const_defined?("ModuleA").should == true
    ConstantSpecs.const_defined?("ClassA").should == true
    ConstantSpecs::ContainerA.const_defined?("ChildA").should == true
  end

  it "returns true when passed a constant name with unicode characters" do
    ConstantUnicodeSpecs.const_defined?("CS_CONSTλ").should be_true
  end

  # NATFIXME: Multibyte in EUC_JP
  xit "returns true when passed a constant name with EUC-JP characters" do
    str = "CS_CONSTλ".encode("euc-jp")
    ConstantSpecs.const_set str, 1
    ConstantSpecs.const_defined?(str).should be_true
  end

  it "returns false if the constant is not defined in the receiver, its superclass, or any included modules" do
    # The following constant isn't defined at all.
    ConstantSpecs::ContainerA::ChildA.const_defined?(:CS_CONST4726).should be_false
    # DETACHED_CONSTANT is defined in ConstantSpecs::Detached, which isn't
    # included by or inherited from ParentA
    ConstantSpecs::ParentA.const_defined?(:DETACHED_CONSTANT).should be_false
  end

  it "does not call #const_missing if the constant is not defined in the receiver" do
    ConstantSpecs::ClassA.should_not_receive(:const_missing)
    ConstantSpecs::ClassA.const_defined?(:CS_CONSTX).should == false
  end

  describe "converts the given name to a String using #to_str" do
    it "calls #to_str to convert the given name to a String" do
      name = mock("ClassA")
      name.should_receive(:to_str).and_return("ClassA")
      ConstantSpecs.const_defined?(name).should == true
    end

    it "raises a TypeError if the given name can't be converted to a String" do
      -> { ConstantSpecs.const_defined?(nil) }.should raise_error(TypeError)
      -> { ConstantSpecs.const_defined?([])  }.should raise_error(TypeError)
    end

    it "raises a NoMethodError if the given argument raises a NoMethodError during type coercion to a String" do
      name = mock("classA")
      name.should_receive(:to_str).and_raise(NoMethodError)
      -> { ConstantSpecs.const_defined?(name) }.should raise_error(NoMethodError)
    end
  end

  it "special cases Object and checks it's included Modules" do
    NATFIXME "special cases Object and checks it's included Modules", exception: SpecFailedException do
      Object.const_defined?(:CS_CONST10).should be_true
    end
  end

  it "returns true for toplevel constant when the name begins with '::'" do
    NATFIXME 'Scoping', exception: SpecFailedException do
      ConstantSpecs.const_defined?("::Array").should be_true
    end
  end

  it "returns true when passed a scoped constant name" do
    NATFIXME 'Scoping', exception: SpecFailedException do
      ConstantSpecs.const_defined?("ClassC::CS_CONST1").should be_true
    end
  end

  it "returns true when passed a scoped constant name for a constant in the inheritance hierarchy and the inherited flag is default" do
    NATFIXME 'Scoping', exception: SpecFailedException do
      ConstantSpecs::ClassD.const_defined?("ClassE::CS_CONST2").should be_true
    end
  end

  it "returns true when passed a scoped constant name for a constant in the inheritance hierarchy and the inherited flag is true" do
    NATFIXME 'Support inherited argument', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
      ConstantSpecs::ClassD.const_defined?("ClassE::CS_CONST2", true).should be_true
    end
  end

  it "returns false when passed a scoped constant name for a constant in the inheritance hierarchy and the inherited flag is false" do
    NATFIXME 'Support inherited argument', exception: ArgumentError, message: 'wrong number of arguments (given 2, expected 1)' do
      ConstantSpecs::ClassD.const_defined?("ClassE::CS_CONST2", false).should be_false
    end
  end

  it "returns false when the name begins with '::' and the toplevel constant does not exist" do
    ConstantSpecs.const_defined?("::Name").should be_false
  end

  it "raises a NameError if the name does not start with a capital letter" do
    NATFIXME 'Validate name', exception: SpecFailedException do
      -> { ConstantSpecs.const_defined? "name" }.should raise_error(NameError)
    end
  end

  it "raises a NameError if the name starts with '_'" do
    NATFIXME 'Validate name', exception: SpecFailedException do
      -> { ConstantSpecs.const_defined? "__CONSTX__" }.should raise_error(NameError)
    end
  end

  it "raises a NameError if the name starts with '@'" do
    NATFIXME 'Validate name', exception: SpecFailedException do
      -> { ConstantSpecs.const_defined? "@Name" }.should raise_error(NameError)
    end
  end

  it "raises a NameError if the name starts with '!'" do
    NATFIXME 'Validate name', exception: SpecFailedException do
      -> { ConstantSpecs.const_defined? "!Name" }.should raise_error(NameError)
    end
  end

  it "returns true or false for the nested name" do
    ConstantSpecs.const_defined?("NotExist::Name").should == false
    ConstantSpecs.const_defined?("::Name").should == false
    NATFIXME 'Scoping', exception: SpecFailedException do
      ConstantSpecs.const_defined?("::Object").should == true
      ConstantSpecs.const_defined?("ClassA::CS_CONST10").should == true
    end
    ConstantSpecs.const_defined?("ClassA::CS_CONST10_").should == false
  end

  it "raises a NameError if the name contains non-alphabetic characters except '_'" do
    ConstantSpecs.const_defined?("CS_CONSTX").should == false
    NATFIXME 'Validate name', exception: SpecFailedException do
      -> { ConstantSpecs.const_defined? "Name=" }.should raise_error(NameError)
      -> { ConstantSpecs.const_defined? "Name?" }.should raise_error(NameError)
    end
  end

  it "raises a TypeError if conversion to a String by calling #to_str fails" do
    name = mock('123')
    -> { ConstantSpecs.const_defined? name }.should raise_error(TypeError)

    name.should_receive(:to_str).and_return(123)
    -> { ConstantSpecs.const_defined? name }.should raise_error(TypeError)
  end
end
