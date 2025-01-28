# frozen_string_literal: true

require 'etc'

class Dir
  def self.tmpdir
    ENV.fetch('TMPDIR') { Etc.systmpdir }
  end
end
