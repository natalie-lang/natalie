require_relative './AST_based/canary_array_functions'
require_relative './AST_based/constant_folding'

# FIXME: Some of these  might be a bit more powerful if we implement them using
#        the stack-based IR
# FIXME: These are still dependent on the original compilers BasePass
module Natalie
  class Compiler2
    AST_BASED_OPTIMIZATIONS = [
      {
        name: 'canary_array_functions',
        class: CanaryArrayFunctions,
        optimization_level: 0,
        disable_flags: %w[nocanarry_array_functions],
      },
      {
        name: 'constant_folding',
        class: ConstantFolding,
        optimization_level: 1,
        enable_flags: %w[cf],
        disable_flags: %w[nocf],
      },
    ]
  end
end
