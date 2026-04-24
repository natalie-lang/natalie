# IO#wait, #wait_readable, and #wait_writable are already provided as built-in
# methods on IO (see lib/natalie/compiler/binding_gen.rb). This file exists so
# that `require 'io/wait'` succeeds, matching MRI where io/wait is a C
# extension that adds these methods to IO.
