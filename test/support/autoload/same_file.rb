module Foo
  class SameFile1
  end

  class SameFile2
  end
end

$same_file_loaded ||= 0
$same_file_loaded += 1
