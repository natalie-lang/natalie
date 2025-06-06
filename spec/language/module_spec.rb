require_relative '../spec_helper'
require_relative 'fixtures/module'

describe "The module keyword" do
  it "creates a new module without semicolon" do
    module ModuleSpecsKeywordWithoutSemicolon end
    ModuleSpecsKeywordWithoutSemicolon.should be_an_instance_of(Module)
  end

  it "creates a new module with a non-qualified constant name" do
    module ModuleSpecsToplevel; end
    ModuleSpecsToplevel.should be_an_instance_of(Module)
  end

  it "creates a new module with a qualified constant name" do
    module ModuleSpecs::Nested; end
    ModuleSpecs::Nested.should be_an_instance_of(Module)
  end

  it "creates a new module with a variable qualified constant name" do
    m = Module.new
    module m::N; end
    m::N.should be_an_instance_of(Module)
  end

  it "reopens an existing module" do
    module ModuleSpecs; Reopened = true; end
    ModuleSpecs::Reopened.should be_true
  ensure
    ModuleSpecs.send(:remove_const, :Reopened)
  end

  it "does not reopen a module included in Object" do
    module IncludedModuleSpecs; Reopened = true; end
    NATFIXME 'does not reopen a module included in Object', exception: SpecFailedException do
      ModuleSpecs::IncludedInObject::IncludedModuleSpecs.should_not == Object::IncludedModuleSpecs
    end
  ensure
    IncludedModuleSpecs.send(:remove_const, :Reopened)
  end

  it "raises a TypeError if the constant is a Class" do
    -> do
      module ModuleSpecs::Modules::Klass; end
    end.should raise_error(TypeError)
  end

  it "raises a TypeError if the constant is a String" do
    -> { module ModuleSpecs::Modules::A; end }.should raise_error(TypeError)
  end

  it "raises a TypeError if the constant is an Integer" do
    -> { module ModuleSpecs::Modules::B; end }.should raise_error(TypeError)
  end

  it "raises a TypeError if the constant is nil" do
    -> { module ModuleSpecs::Modules::C; end }.should raise_error(TypeError)
  end

  it "raises a TypeError if the constant is true" do
    -> { module ModuleSpecs::Modules::D; end }.should raise_error(TypeError)
  end

  it "raises a TypeError if the constant is false" do
    -> { module ModuleSpecs::Modules::D; end }.should raise_error(TypeError)
  end
end

describe "Assigning an anonymous module to a constant" do
  it "sets the name of the module" do
    mod = Module.new
    mod.name.should be_nil

    ::ModuleSpecs_CS1 = mod
    mod.name.should == "ModuleSpecs_CS1"
  ensure
    Object.send(:remove_const, :ModuleSpecs_CS1)
  end

  it "sets the name of a module scoped by an anonymous module" do
    a, b = Module.new, Module.new
    a::B = b
    NATFIXME 'sets the name of a module scoped by an anonymous module', exception: SpecFailedException do
      b.name.should.end_with? '::B'
    end
  end

  it "sets the name of contained modules when assigning a toplevel anonymous module" do
    a, b, c, d = Module.new, Module.new, Module.new, Module.new
    a::B = b
    a::B::C = c
    a::B::C::E = c
    a::D = d

    ::ModuleSpecs_CS2 = a
    a.name.should == "ModuleSpecs_CS2"
    b.name.should == "ModuleSpecs_CS2::B"
    NATFIXME 'sets the name of contained modules when assigning a toplevel anonymous module', exception: SpecFailedException do
      c.name.should == "ModuleSpecs_CS2::B::C"
    end
    d.name.should == "ModuleSpecs_CS2::D"
  ensure
    Object.send(:remove_const, :ModuleSpecs_CS2)
  end
end
