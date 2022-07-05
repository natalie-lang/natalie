require_relative 'support/spec'
require_relative 'support/nat_binary'

def compiler2?
  NATALIE_COMPILER == 'v2'
rescue NameError
  false
end
