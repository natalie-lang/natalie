# frozen_string_literal: true

require 'natalie/inline'
require 'etc' # For Dir.tmp fallback to Etc.systmpdir
require 'tmpdir.cpp'
require 'fileutils'

class Dir
  __bind_static_method__ :create_tmpdir, :Dir_mktmpdir
  __bind_static_method__ :tmpdir, :Dir_tmpdir

  def self.mktmpdir(*args)
    path = create_tmpdir(*args)
    return path unless block_given?

    begin
      yield path
    ensure
      FileUtils.rm_rf(path)
    end
  end

  class << self
    private :create_tmpdir
  end
end
