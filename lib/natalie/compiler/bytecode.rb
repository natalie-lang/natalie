module Natalie
  class Compiler
    module Bytecode
      # Current version is 0.0, which means we do not guarantee any backwards compatibility
      MAJOR_VERSION = 0
      MINOR_VERSION = 0

      SECTIONS = {
        1 => :CODE,
        2 => :RODATA,
      }.freeze
    end
  end
end
