$autoload_retry_attempts = ($autoload_retry_attempts || 0) + 1
raise 'autoload first attempt' if $autoload_retry_attempts == 1

module Foo
  Retry = :loaded
end
