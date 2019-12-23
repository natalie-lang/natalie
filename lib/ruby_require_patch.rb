module Kernel
  define_method(:require_relative) do |filename|
    load File.expand_path(File.join('../examples', filename + '.nat'), __dir__)
  end
end
