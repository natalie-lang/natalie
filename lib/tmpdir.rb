# frozen_string_literal: true

require 'natalie/inline'
require 'etc.cpp' # For Dir.tmp fallback to Etc.systmpdir
require 'tmpdir.cpp'

class Dir
  __bind_static_method__ :tmpdir, :Dir_tmpdir
end
