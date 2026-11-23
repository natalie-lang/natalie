module Foo
  Early = :loaded
end

begin
  return
ensure
  $autoload_early_ensure = true
end
