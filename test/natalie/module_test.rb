require_relative '../spec_helper'

module M1
end

module M2
  include M1
end

class C1
  include M1
end

module M3
  include M2
  module M3A
    def m3a
      'm3a'
    end
  end
  class A
    class << self
      include M3A
    end
  end
end

class C2 < C1
end

describe 'Module' do
  describe '.include?' do
    it 'returns true if the module includes the given module' do
      M2.include?(M2).should == false
      M2.include?(M1).should == true
      M1.include?(M2).should == false
      M3.include?(M1).should == true
      C1.include?(M1).should == true
      C1.include?(M2).should == false
      C2.include?(M1).should == true
      -> { C1.include?(nil) }.should raise_error(TypeError)
    end
  end

  describe 'include' do
    it 'works on the singleton class' do
      M3::A.m3a.should == 'm3a'
    end
  end

  describe '#const_get' do
    it 'returns a constant by name' do
      Object.const_get(:M3).should == M3
      M3.const_get('A').should == M3::A
    end
  end

  describe '#instance_eval' do
    it 'works' do
      M1.instance_eval do |m1|
        self.should == M1
        m1.should == M1
      end
    end
  end
end
