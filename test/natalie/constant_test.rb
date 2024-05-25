require_relative '../spec_helper'

FOO = 1
Bar = 2

class Object
  CONST_IN_OBJECT = :object
end

module Kernel
  CONST_IN_KERNEL = :kernel
end

module Mixin
  CONST_IN_MIXIN = :mixin

  def mixin_method
    CONST_IN_MIXIN
  end
end

class TheSuperclass
  include Mixin
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
  CONST_IN_MODULE_C = :module_c
end

class TheClass < TheSuperclass
  include ModuleA
  include ModuleC

  def mixin_constant
    mixin_method
  end

  def object_const
    CONST_IN_OBJECT
  end

  def kernel_const
    CONST_IN_KERNEL
  end
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

  it 'does not find the constant on the Object class' do
    -> { TheClass::CONST_IN_OBJECT }.should raise_error(NameError)
  end

  it 'does find constant from Object class if referenced in other class' do
    TheClass.new.object_const.should == :object
  end

  it 'does find constant from Kernel module if referenced in other class' do
    TheClass.new.kernel_const.should == :kernel
  end

  it 'finds constants in a mixin when resolved from a subclass' do
    TheClass.new.mixin_constant.should == :mixin
  end

  it 'raises NameError for missing const' do
    -> { UnknownConst; nil }.should raise_error(NameError, /uninitialized constant UnknownConst/)
  end

  describe 'using &&= assignment (Prism::ConstantAndWriteNode)' do
    it 'can assign a value' do
      module ModuleA
        -> { QUUX &&= nil }.should raise_error(NameError, /uninitialized constant ModuleA::QUUX/)

        QUUX = nil
        QUUX &&= 1
        QUUX.should be_nil
        remove_const(:QUUX)

        QUUX = 1
        suppress_warning { QUUX &&= 2 }
        QUUX.should == 2
        remove_const(:QUUX)
      end
    end
  end

  describe 'using += write (Prism::ConstantOperatorWriteNode)' do
    it 'can change a value' do
      module ModuleA
        QUUX = 1
        suppress_warning { QUUX += 1 }
        QUUX.should == 2
        remove_const(:QUUX)
      end
    end
  end

  describe 'using ||= assignment (Prism::ConstantOrWriteNode)' do
    it 'can create and assign a value' do
      module ModuleA
        -> { QUUX }.should raise_error(NameError, /uninitialized constant ModuleA::QUUX/)

        QUUX ||= nil
        QUUX.should be_nil

        suppress_warning { QUUX ||= false }
        QUUX.should be_false

        suppress_warning { QUUX ||= 1 }
        QUUX.should == 1

        suppress_warning { QUUX ||= 2 }
        QUUX.should == 1

        remove_const(:QUUX)
      end
    end

    it 'has the correct return values' do
      module ModuleA
        (QUUX ||= nil).should be_nil

        suppress_warning do
          (QUUX ||= false).should be_false
        end

        suppress_warning do
          (QUUX ||= 1).should == 1
        end

        suppress_warning do
          (QUUX ||= 2).should == 1
        end

        remove_const(:QUUX)
      end
    end
  end

  describe 'using += write (Prism::ConstantPathOperatorWriteNode)' do
    it 'can change a value' do
      module ModuleA
        QUUX = 1
      end
      suppress_warning { ModuleA::QUUX += 1 }
      ModuleA::QUUX.should == 2

      suppress_warning do
        (ModuleA::QUUX += 1).should == 3
      end

      ModuleA.send(:remove_const, :QUUX)
    end
  end
end
