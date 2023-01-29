module BasicObjectSpecs
  class BOSubclass < BasicObject
    def self.kernel_defined?
      defined?(Kernel)
    end

    # NATFIXME: This results in "wrong number of arguments (given 0, expected 1+) (ArgumentError)"
    # include ::Kernel
    include(::Kernel)
  end
end
