require 'natalie/inline'
require 'io/nonblock.cpp'

class IO
  __bind_method__ :nonblock?, :IO_is_nonblock
  __bind_method__ :nonblock=, :IO_set_nonblock
end
