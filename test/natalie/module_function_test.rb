require_relative '../spec_helper'

describe 'Module#module_function' do
  it 'preserves captured locals and parameter metadata' do
    mod =
      Module.new do
        offset = 40
        define_method(:calculate) { |value, increment: 1| offset + value + increment }
        module_function :calculate
      end
    GC.start
    mod.calculate(1, increment: 2).should == 43
    mod.method(:calculate).parameters.should == [%i[req value], %i[key increment]]
  end

  it 'preserves parameter names when enabled before a method definition' do
    mod =
      Module.new do
        module_function
        def calculate(value, increment: 1)
          value + increment
        end
      end
    mod.calculate(1, increment: 2).should == 3
    mod.method(:calculate).parameters.should == [%i[req value], %i[key increment]]
  end
end
