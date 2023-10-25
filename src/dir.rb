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

    # FIXME: we accept multiple patterns, but really this breaks if the patterns aren't very similar
    # e.g. combining "/foo/**/*.rb" with "./*.rb" is going to break.
    # We should run through the recurse process for each pattern I think.
    def glob(patterns, flags = 0, base: nil, sort: true)
      if sort != true && sort != false
        raise ArgumentError, "expected true or false as sort: #{sort.inspect}"
      end

      unless block_given?
        result = to_enum(:glob, patterns, flags, base: base).to_a
        return sort ? result.sort : result
      end

      patterns = Array(patterns).map do |pattern|
        if pattern.is_a?(String)
          pattern
        elsif pattern.respond_to?(:to_path)
          pattern.to_path
        end
      end

      is_absolute = patterns.first.start_with?(File::SEPARATOR)
      flags |= File::FNM_PATHNAME | File::FNM_EXTGLOB
      base_given = !(base.nil? || base == '')

      unless base_given
        if is_absolute
          longest_real_path = patterns.first.split(File::SEPARATOR).take_while do |part|
            part !~ /\*|\?|\[|\{/
          end
          base = longest_real_path.join(File::SEPARATOR)
          base = File.split(base).first unless File.directory?(base)
        else
          base = Dir.pwd unless base_given
        end
      end
      return unless File.directory?(base)

      raise ArgumentError, 'nul-separated glob pattern' if patterns.any? { |pat| pat.include?("\0") }

      follow_symlinks = patterns.grep(/(\A|[^*])\*#{File::SEPARATOR}/).any?
      start_with_dot = patterns.grep(/\A\./).any?
      dot_slash = patterns.grep(/\A\.#{File::SEPARATOR}/).any?
      end_with_slash = patterns.first.end_with?(File::SEPARATOR)

      recurse = lambda do |dir|
        dir_to_match = dir
        if is_absolute
          dir_to_match = File.join(base, dir)
        elsif end_with_slash && !dir.end_with?('/')
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
          relative_path = File.join(dir, path)
          relative_path.sub!(%r{^\./}, '') unless dot_slash
          path_to_match = is_absolute ? File.join(base, relative_path) : relative_path
          if File.directory?(relative_path)
            recurse.(relative_path)
          elsif patterns.any? { |pattern| File.fnmatch(pattern, path_to_match, flags) }
            yield path_to_match
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
