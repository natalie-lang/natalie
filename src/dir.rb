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
      base_given = !!base
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

      follow_symlinks = patterns.grep(/(\A|[^\*])\*#{File::SEPARATOR}/).any?
      start_with_dot = patterns.grep(/\A\./).any?
      dot_slash = patterns.grep(%r{\A\./}).any?
      end_with_slash = patterns.first.end_with?('/')

      recurse = lambda do |dir|
        dir_to_match = dir
        if end_with_slash && !dir.end_with?('/')
          if dir == '.' && !start_with_dot
            dir_to_match = '/' if base_given
          else
            dir_to_match = dir + '/'
          end
        end

        return if File.symlink?(dir) && !follow_symlinks

        if patterns.any? { |pattern| File.fnmatch(pattern, dir_to_match, flags) }
          yield dir_to_match
        end

        Dir.children(dir)&.each do |path|
          full_path = File.join(dir, path)
          full_path.sub!(%r{^\./}, '') unless dot_slash
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

      nil
    end

    alias [] glob
  end
end
