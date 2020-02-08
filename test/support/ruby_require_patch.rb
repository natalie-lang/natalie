module Kernel
  define_method(:require) do |filename|
    load filename + '.nat'
  end

  define_method(:require_relative) do |filename|
    path = File.split(caller.first.split(':').first).first
    load File.join(path, filename + '.nat')
  end
end
