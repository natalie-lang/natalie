require_relative '../spec_helper'

Con = :T

module SP1P1
  Con = :SP1P1
end

module SP1P2
  Con = :SP1P2
end

module SP1M1
  Con = :SP1M1
end

module SP1M2
  Con = :SP1M2
end

module SP1
  prepend SP1P1
  prepend SP1P2
  include SP1M1
  include SP1M2

  Con = :SP1
end

module SP2
  Con = :SP2
end

module SM1
  Con = :SM1
end

module SM2
  Con = :SM2
end

class S
  prepend SP1
  prepend SP2
  include SM1
  include SM2

  Con = :S
end

module CP1
  Con = :CP1
end

module CP2
  Con = :CP2
end

module CM1
  Con = :CM1
end

module CM2
  Con = :CM2
end

module LP1
  Con = :LP1
end

module LP2
  Con = :LP2
end

module LM1
  Con = :LM1
end

module LM2
  Con = :LM2
end

class L
  prepend LP1
  prepend LP2
  include LM1
  include LM2

  Con = :L

  class C < S
    prepend CP1
    prepend CP2
    include CM1
    include CM2

    Con = :C

    def c = Con
  end
end

describe 'constant look-up order' do
  it 'finds name-clashing constants in the correct order' do
    L::C.new.c.should == :C
    L::C.send(:remove_const, :Con)

    L::C.new.c.should == :L
    L.send(:remove_const, :Con)

    L::C.new.c.should == :CP2
    CP2.send(:remove_const, :Con)

    L::C.new.c.should == :CP1
    CP1.send(:remove_const, :Con)

    L::C.new.c.should == :CM2
    CM2.send(:remove_const, :Con)

    L::C.new.c.should == :CM1
    CM1.send(:remove_const, :Con)

    L::C.new.c.should == :SP2
    SP2.send(:remove_const, :Con)

    L::C.new.c.should == :SP1P2
    SP1P2.send(:remove_const, :Con)

    L::C.new.c.should == :SP1P1
    SP1P1.send(:remove_const, :Con)

    L::C.new.c.should == :SP1
    SP1.send(:remove_const, :Con)

    L::C.new.c.should == :SP1M2
    SP1M2.send(:remove_const, :Con)

    L::C.new.c.should == :SP1M1
    SP1M1.send(:remove_const, :Con)

    L::C.new.c.should == :S
    S.send(:remove_const, :Con)

    L::C.new.c.should == :SM2
    SM2.send(:remove_const, :Con)

    L::C.new.c.should == :SM1
    SM1.send(:remove_const, :Con)

    L::C.new.c.should == :T
  end
end

class M1
  Con1 = :M1

  class C1
    def c1 = Con1
  end
end

class M1::C1
  def c2 = Con1
end

describe 'constant look-up order' do
  it 'finds constant only in correct lexical scope' do
    M1::C1.new.c1.should == :M1
    -> { M1::C1.new.c2 }.should raise_error(NameError, /uninitialized constant M1::C1::Con1/)
  end
end
