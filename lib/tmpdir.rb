# frozen_string_literal: true

require 'natalie/inline'
require 'etc' # For Dir.tmp fallback to Etc.systmpdir
require 'tmpdir.cpp'
require 'fileutils'

class Dir
  __bind_static_method__ :mktmpdir, :Dir_mktmpdir
  __bind_static_method__ :tmpdir, :Dir_tmpdir
end
