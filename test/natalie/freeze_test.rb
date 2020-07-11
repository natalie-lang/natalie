require_relative '../spec_helper'

class FooClass
end

module BarModule
end

describe 'Kernel#freeze' do
  it 'prevents an Array from being modified' do
    a = [1, 2, 3]
    a.freeze
    -> { a << 4 }.should raise_error(FrozenError)
    -> { a[0] = 'one' }.should raise_error(FrozenError)
    -> { a.pop }.should raise_error(FrozenError)
  end

  it 'prevents a Hash from being modified' do
    h = { 1 => 1, 2 => 2 }
    h.freeze
    -> { h[3] = 3 }.should raise_error(FrozenError)
    -> { h.delete(1) }.should raise_error(FrozenError)
  end

  it 'prevents a String from being modified' do
    s = 'foo'
    s.freeze
    -> { s << 'bar' }.should raise_error(FrozenError)
  end

  # it 'prevents a Class from being modified' do
  #   FooClass.freeze
  #   -> {
  #     class FooClass
  #       def foo
  #       end
  #     end
  #   }.should raise_error(FrozenError)
  #   -> {
  #     FooClass.class_eval do
  #       def foo
  #       end
  #     end
  #   }.should raise_error(FrozenError)
  # end

  # it 'prevents a Module from being modified' do
  #   BarModule.freeze
  #   -> {
  #     module BarModule
  #       def bar
  #       end
  #     end
  #   }.should raise_error(FrozenError)
  #   -> {
  #     BarModule.class_eval do
  #       def foo
  #       end
  #     end
  #   }.should raise_error(FrozenError)
  # end
end
