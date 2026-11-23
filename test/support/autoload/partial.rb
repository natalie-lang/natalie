$autoload_partial_attempts = ($autoload_partial_attempts || 0) + 1

module Foo
  Partial = $autoload_partial_attempts
end

raise 'autoload after assignment' if $autoload_partial_attempts == 1
