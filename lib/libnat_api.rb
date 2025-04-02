require 'natalie/inline'
require_relative 'natalie'

# This is a pretty simple "C API" which basically just loads up all of the
# Natalie Parser and Compiler inside the calling process. This allows us to
# build a libnat.so shared object that can be loaded and initialized from
# another, much smaller Natalie program. For examples of this usage, see:
#
# - test/natalie/libnat_test.rb
# - lib/natalie/repl.rb

__inline__ <<~END
  Value init_libnat(Env *, Value);

  // Not very clever, but reversing the name lets us expose this
  // similarly-named function via C linkage.
  extern "C" void libnat_init(Env *env, Object *self) {
      init_libnat(env, self);
  }
END
