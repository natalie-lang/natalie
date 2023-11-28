require_relative '../../spec_helper'
require_relative '../../fixtures/constants'
require_relative 'fixtures/constants_autoload'

describe "Module#const_get" do
  it "accepts a String or Symbol name" do
    Object.const_get(:CS_CONST1).should == :const1
    Object.const_get("CS_CONST1").should == :const1
  end

  it "raises a NameError if no constant is defined in the search path" do
    -> { ConstantSpecs.const_get :CS_CONSTX }.should raise_error(NameError)
  end

  it "raises a NameError with the not found constant symbol" do
    NATFIXME 'raises a NameError with the not found constant symbol', exception: SpecFailedException do
      error_inspection = -> e { e.name.should == :CS_CONSTX }
      -> { ConstantSpecs.const_get :CS_CONSTX }.should raise_error(NameError, &error_inspection)
    end
  end

  it "raises a NameError if the name does not start with a capital letter" do
    -> { ConstantSpecs.const_get "name" }.should raise_error(NameError)
  end

  it "raises a NameError if the name starts with a non-alphabetic character" do
    -> { ConstantSpecs.const_get "__CONSTX__" }.should raise_error(NameError)
    -> { ConstantSpecs.const_get "@CS_CONST1" }.should raise_error(NameError)
    -> { ConstantSpecs.const_get "!CS_CONST1" }.should raise_error(NameError)
  end

  it "raises a NameError if the name contains non-alphabetic characters except '_'" do
    Object.const_get("CS_CONST1").should == :const1
    -> { ConstantSpecs.const_get "CS_CONST1=" }.should raise_error(NameError)
    -> { ConstantSpecs.const_get "CS_CONST1?" }.should raise_error(NameError)
  end

  it "calls #to_str to convert the given name to a String" do
    name = mock("ClassA")
    name.should_receive(:to_str).and_return("ClassA")
    ConstantSpecs.const_get(name).should == ConstantSpecs::ClassA
  end

  it "raises a TypeError if conversion to a String by calling #to_str fails" do
    name = mock('123')
    -> { ConstantSpecs.const_get(name) }.should raise_error(TypeError)

    name.should_receive(:to_str).and_return(123)
    -> { ConstantSpecs.const_get(name) }.should raise_error(TypeError)
  end

  it "calls #const_missing on the receiver if unable to locate the constant" do
    ConstantSpecs::ContainerA.should_receive(:const_missing).with(:CS_CONSTX)
    ConstantSpecs::ContainerA.const_get(:CS_CONSTX)
  end

  it "does not search the singleton class of a Class or Module" do
    -> do
      ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST14)
    end.should raise_error(NameError)
    -> { ConstantSpecs.const_get(:CS_CONST14) }.should raise_error(NameError)
  end

  it "does not search the containing scope" do
    NATFIXME 'does not search the containing scope', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerA::ChildA::CS_CONST20' do
      ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST20).should == :const20_2
      -> do
        ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST5)
      end.should raise_error(NameError)
    end
  end

  it "raises a NameError if the constant is defined in the receiver's superclass and the inherit flag is false" do
    -> do
      ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST4, false)
    end.should raise_error(NameError)
  end

  it "searches into the receiver superclasses if the inherit flag is true" do
    NATFIXME 'Behaviour is the same as without the boolean flag', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerA::ChildA::CS_CONST4' do
      ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST4, true).should == :const4
    end
  end

  it "raises a NameError when the receiver is a Module, the constant is defined at toplevel and the inherit flag is false" do
    -> do
      ConstantSpecs::ModuleA.const_get(:CS_CONST1, false)
    end.should raise_error(NameError)
  end

  it "raises a NameError when the receiver is a Class, the constant is defined at toplevel and the inherit flag is false" do
    -> do
      ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST1, false)
    end.should raise_error(NameError)
  end

  it "coerces the inherit flag to a boolean" do
    NATFIXME 'Behaviour is the same as without the boolean flag', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerA::ChildA::CS_CONST4' do
      ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST4, :true).should == :const4
    end

    -> do
      ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST1, nil)
    end.should raise_error(NameError)
  end

  it "accepts a toplevel scope qualifier" do
    NATFIXME 'accepts a toplevel scope qualifier', exception: NameError, message: 'uninitialized constant ConstantSpecs::::CS_CONST1' do
      ConstantSpecs.const_get("::CS_CONST1").should == :const1
    end
  end

  it "accepts a toplevel scope qualifier when inherit is false" do
    NATFIXME 'accepts a toplevel scope qualifier', exception: NameError, message: 'uninitialized constant ::CS_CONST1' do
      ConstantSpecs.const_get("::CS_CONST1", false).should == :const1
      -> { ConstantSpecs.const_get("CS_CONST1", false) }.should raise_error(NameError)
    end
  end

  it "returns a constant whose module is defined the toplevel" do
    NATFIXME 'returns a constant whose module is defined the toplevel', exception: NameError, message: 'uninitialized constant ConstantSpecs::ConstantSpecsTwo::Foo' do
      ConstantSpecs.const_get("ConstantSpecsTwo::Foo").should == :cs_two_foo
      ConstantSpecsThree.const_get("ConstantSpecsTwo::Foo").should == :cs_three_foo
    end
  end

  it "accepts a scoped constant name" do
    NATFIXME 'accepts a scoped constant name', exception: NameError, message: 'uninitialized constant ConstantSpecs::ClassA::CS_CONST10' do
      ConstantSpecs.const_get("ClassA::CS_CONST10").should == :const10_10
    end
  end

  it "raises a NameError if the name includes two successive scope separators" do
    -> { ConstantSpecs.const_get("ClassA::::CS_CONST10") }.should raise_error(NameError)
  end

  it "raises a NameError if only '::' is passed" do
    -> { ConstantSpecs.const_get("::") }.should raise_error(NameError)
  end

  it "raises a NameError if a Symbol has a toplevel scope qualifier" do
    -> { ConstantSpecs.const_get(:'::CS_CONST1') }.should raise_error(NameError)
  end

  it "raises a NameError if a Symbol is a scoped constant name" do
    -> { ConstantSpecs.const_get(:'ClassA::CS_CONST10') }.should raise_error(NameError)
  end

  it "does read private constants" do
     ConstantSpecs.const_get(:CS_PRIVATE).should == :cs_private
  end

  # NATFIXME: Failed assertion in include/natalie/constant.hpp:20
  xit 'does autoload a constant' do
    Object.const_get('CSAutoloadA').name.should == 'CSAutoloadA'
  end

  it 'does autoload a constant with a toplevel scope qualifier' do
    NATFIXME 'does autoload a constant with a toplevel scope qualifier', exception: NameError, message: 'uninitialized constant ::CSAutoloadB' do
      Object.const_get('::CSAutoloadB').name.should == 'CSAutoloadB'
    end
  end

  it 'does autoload a module and resolve a constant within' do
    NATFIXME 'does autoload a module and resolve a constant within', exception: NameError, message: 'uninitialized constant CSAutoloadC::CONST' do
      Object.const_get('CSAutoloadC::CONST').should == 7
    end
  end

  it 'does autoload a non-toplevel module' do
    NATFIXME 'does autoload a non-toplevel module', exception: NameError, message: 'uninitialized constant CSAutoloadD::InnerModule' do
      Object.const_get('CSAutoloadD::InnerModule').name.should == 'CSAutoloadD::InnerModule'
    end
  end

  it "raises a NameError when the nested constant does not exist on the module but exists in Object" do
    -> { Object.const_get('ConstantSpecs::CS_CONST1') }.should raise_error(NameError)
  end

  describe "with statically assigned constants" do
    it "searches the immediate class or module first" do
      ConstantSpecs::ClassA.const_get(:CS_CONST10).should == :const10_10
      ConstantSpecs::ModuleA.const_get(:CS_CONST10).should == :const10_1
      ConstantSpecs::ParentA.const_get(:CS_CONST10).should == :const10_5
      ConstantSpecs::ContainerA.const_get(:CS_CONST10).should == :const10_2
      ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST10).should == :const10_3
    end

    it "searches a module included in the immediate class before the superclass" do
      NATFIXME 'searches a module included in the immediate class before the superclass', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerA::ChildA::CS_CONST15' do
        ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST15).should == :const15_1
      end
    end

    it "searches the superclass before a module included in the superclass" do
      NATFIXME 'searches the superclass before a module included in the superclass', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerA::ChildA::CS_CONST11' do
        ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST11).should == :const11_1
      end
    end

    it "searches a module included in the superclass" do
      NATFIXME 'searches a module included in the superclass', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerA::ChildA::CS_CONST12' do
        ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST12).should == :const12_1
      end
    end

    it "searches the superclass chain" do
      NATFIXME 'searches the superclass chain', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerA::ChildA::CS_CONST13' do
        ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST13).should == :const13
      end
    end

    it "returns a toplevel constant when the receiver is a Class" do
      NATFIXME 'returns a toplevel constant when the receiver is a Class', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerA::ChildA::CS_CONST1' do
        ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST1).should == :const1
      end
    end

    it "returns a toplevel constant when the receiver is a Module" do
      NATFIXME 'returns a toplevel constant when the receiver is a Module', exception: NameError, message: 'uninitialized constant ConstantSpecs::CS_CONST1' do
        ConstantSpecs.const_get(:CS_CONST1).should == :const1
        ConstantSpecs::ModuleA.const_get(:CS_CONST1).should == :const1
      end
    end
  end

  describe "with dynamically assigned constants" do
    it "searches the immediate class or module first" do
      ConstantSpecs::ClassA::CS_CONST301 = :const301_1
      ConstantSpecs::ClassA.const_get(:CS_CONST301).should == :const301_1

      ConstantSpecs::ModuleA::CS_CONST301 = :const301_2
      ConstantSpecs::ModuleA.const_get(:CS_CONST301).should == :const301_2

      ConstantSpecs::ParentA::CS_CONST301 = :const301_3
      ConstantSpecs::ParentA.const_get(:CS_CONST301).should == :const301_3

      ConstantSpecs::ContainerA::ChildA::CS_CONST301 = :const301_5
      ConstantSpecs::ContainerA::ChildA.const_get(:CS_CONST301).should == :const301_5
    end

    it "searches a module included in the immediate class before the superclass" do
      ConstantSpecs::ParentB::CS_CONST302 = :const302_1
      ConstantSpecs::ModuleF::CS_CONST302 = :const302_2
      NATFIXME 'searches a module included in the immediate class before the superclass', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerB::ChildB::CS_CONST302' do
        ConstantSpecs::ContainerB::ChildB.const_get(:CS_CONST302).should == :const302_2
      end
    end

    it "searches the superclass before a module included in the superclass" do
      ConstantSpecs::ModuleE::CS_CONST303 = :const303_1
      ConstantSpecs::ParentB::CS_CONST303 = :const303_2
      NATFIXME 'searches the superclass before a module included in the superclass', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerB::ChildB::CS_CONST303' do
        ConstantSpecs::ContainerB::ChildB.const_get(:CS_CONST303).should == :const303_2
      end
    end

    it "searches a module included in the superclass" do
      ConstantSpecs::ModuleA::CS_CONST304 = :const304_1
      ConstantSpecs::ModuleE::CS_CONST304 = :const304_2
      NATFIXME 'searches a module included in the superclass', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerB::ChildB::CS_CONST304' do
        ConstantSpecs::ContainerB::ChildB.const_get(:CS_CONST304).should == :const304_2
      end
    end

    it "searches the superclass chain" do
      ConstantSpecs::ModuleA::CS_CONST305 = :const305
      NATFIXME 'searches the superclass chain', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerB::ChildB::CS_CONST305' do
        ConstantSpecs::ContainerB::ChildB.const_get(:CS_CONST305).should == :const305
      end
    end

    it "returns a toplevel constant when the receiver is a Class" do
      Object::CS_CONST306 = :const306
      NATFIXME 'returns a toplevel constant when the receiver is a Class', exception: NameError, message: 'uninitialized constant ConstantSpecs::ContainerB::ChildB::CS_CONST306' do
        ConstantSpecs::ContainerB::ChildB.const_get(:CS_CONST306).should == :const306
      end
    end

    it "returns a toplevel constant when the receiver is a Module" do
      Object::CS_CONST308 = :const308
      NATFIXME 'returns a toplevel constant when the receiver is a Module', exception: NameError, message: 'uninitialized constant ConstantSpecs::CS_CONST308' do
        ConstantSpecs.const_get(:CS_CONST308).should == :const308
        ConstantSpecs::ModuleA.const_get(:CS_CONST308).should == :const308
      end
    end

    it "returns the updated value of a constant" do
      ConstantSpecs::ClassB::CS_CONST309 = :const309_1
      ConstantSpecs::ClassB.const_get(:CS_CONST309).should == :const309_1

      NATFIXME 'Complain about already initialized constant', exception: SpecFailedException do
        -> {
          ConstantSpecs::ClassB::CS_CONST309 = :const309_2
        }.should complain(/already initialized constant/)
      end
      ConstantSpecs::ClassB.const_get(:CS_CONST309).should == :const309_2
    end
  end
end
