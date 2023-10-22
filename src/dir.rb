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

    def glob(patterns, flags = 0, base: nil, sort: true)
      if sort != true && sort != false
        raise ArgumentError, "expected true or false as sort: #{sort.inspect}"
      end

      unless block_given?
        result = to_enum(:glob, patterns, flags, base: base).to_a
        return sort ? result.sort : result
      end

      flags |= File::FNM_PATHNAME | File::FNM_EXTGLOB
      base = Dir.pwd if base.nil? || base == ''
      return unless File.directory?(base)

      patterns = Array(patterns).map do |pattern|
        if pattern.is_a?(String)
          pattern
        elsif pattern.respond_to?(:to_path)
          pattern.to_path
        end
      end

      raise ArgumentError, 'nul-separated glob pattern' if patterns.any? { |pat| pat.include?("\0") }

      follow_symlinks = true unless patterns.grep(/\*\*/).any?

      recurse = lambda do |dir|
        #p({dir: dir, patterns: patterns})
        dir += '/' if patterns.last.end_with?('/')
        if patterns.any? { |pattern| File.fnmatch(pattern, dir, flags) }
          yield dir
        end
        return if File.symlink?(dir) && !follow_symlinks
        Dir.each_child(dir) do |path|
          full_path = File.join(dir, path).sub(%r{^\./}, '')
          #p({full_path: full_path, patterns: patterns})
          if File.directory?(full_path)
            recurse.(full_path)
          elsif patterns.any? { |pattern| File.fnmatch(pattern, full_path, flags) }
            yield full_path
          end
        end
      rescue Errno::EACCES
        :noop
      end

      Dir.chdir(base) do
        recurse.('.')
      end
    end

    alias [] glob
  end
end
