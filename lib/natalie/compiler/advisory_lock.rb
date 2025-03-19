require 'fileutils'

module Natalie
  class Compiler
    class AdvisoryLock
      def initialize(path)
        @path = path
      end

      def lock
        result = nil
        File.open(@path, File::RDWR | File::CREAT) do |f|
          f.flock(File::LOCK_EX)
          result = yield
        ensure
          f.flock(File::LOCK_UN)
        end
        result
      ensure
        File.unlink(@path) rescue nil
      end
    end
  end
end
