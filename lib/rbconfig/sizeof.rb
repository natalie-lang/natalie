require 'natalie/inline'
require_relative 'sizeof.cpp'

__function__ 'RbConfig_populate_sizes', ['Env *', 'Value'], 'void'

module RbConfig
  SIZEOF = {}
  LIMITS = {}

  __call__('RbConfig_populate_sizes', self)
end
