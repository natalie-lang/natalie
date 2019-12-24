module Kernel
  define_method(:require) do |filename|
    load filename + '.nat'
  end
end
