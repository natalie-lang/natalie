module Foo
  class Nested
    class A < UpALevel # superclass Foo::UpALevel
    end
  end
end

$foo_nested_loaded = true
