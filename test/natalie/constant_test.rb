require_relative '../spec_helper'

FOO = 1
Bar = 2

class TheSuperclass
  CONST_IN_SUPERCLASS = :superclass
  CONST_IN_BOTH_SUPERCLASS_AND_MODULE = :superclass
end

module ModuleB
  CONST_IN_MODULE_B = :module_b
  CONST_IN_MODULE_B_AND_C = :module_b
end

module ModuleA
  include ModuleB
  CONST_IN_MODULE_A = :module_a
  CONST_IN_BOTH_SUPERCLASS_AND_MODULE = :module_a
end

module ModuleC
  CONST_IN_MODULE_B_AND_C = :module_c
end

class TheClass < TheSuperclass
  include ModuleA
  include ModuleC
end

describe 'constants' do
  it 'finds the constant at the top level' do
    FOO.should == 1
    Bar.should == 2
    -> { Baz }.should raise_error(NameError, /uninitialized constant Baz/)
  end

  it 'finds the constant in the superclass' do
    TheClass::CONST_IN_SUPERCLASS.should == :superclass
  end

  it 'finds the constant in included modules' do
    TheClass::CONST_IN_MODULE_A.should == :module_a
    TheClass::CONST_IN_MODULE_B.should == :module_b
  end

  it 'does a breadth-first search through included modules' do
    TheClass::CONST_IN_MODULE_B_AND_C.should == :module_c
  end

  it 'prefers constants from module vs superclass' do
    TheClass::CONST_IN_BOTH_SUPERCLASS_AND_MODULE.should == :module_a
  end
end
