module Foo
  Early = :loaded
end

begin
  return 123
ensure
  $autoload_early_ensure = true
end
