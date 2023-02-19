require 'natalie/inline'

__inline__ '#include <dirent.h>'
__inline__ '#include <sys/param.h>'
__inline__ '#include <sys/types.h>'
__inline__ '#include <unistd.h>'

class Dir
  class << self
    def tmpdir
      '/tmp'
    end

    def each_child(dirname, encoding: nil)
      return enum_for(:each_child, dirname) unless block_given?
      children(dirname, encoding: encoding).each { |name| yield name }
    end
    
    def foreach(dirname, encoding: nil)
      return enum_for(:foreach, dirname) unless block_given?
      entries(dirname, encoding: encoding).each { |name| yield name }
    end

  end
  
end
