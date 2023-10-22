class File
  SEPARATOR = '/'.freeze
  Separator = SEPARATOR
  ALT_SEPARATOR = nil
  PATH_SEPARATOR = ":".freeze

  # NATFIXME: File.join still has many unhandled special cases
  def self.join(*parts)
    parts = parts.flatten
    # pre-validate parts
    parts = parts.map do |item|
      if item.respond_to?(:to_path)
        item = item.to_path
      end
      if item.respond_to?(:to_str)
        item = item.to_str
      end
      unless item.is_a?(String)
        raise TypeError, "no implicit conversion of #{item.class} into String"
      end
      item
    end
    case parts.size
    when 0 then ""
    when 1 then parts[0].dup
    else
      parts.join(SEPARATOR).gsub(/#{SEPARATOR}{2,}/, SEPARATOR)
    end
  end

  def self.split(path)
    [dirname(path), basename(path)]
  end

  def self.basename(path, ext=nil)
    if path.respond_to?(:to_path)
      path = path.to_path
    end
    if path.respond_to?(:to_str)
      path = path.to_str
    end
    raise TypeError, "path must be convertable to String" unless path.is_a?(String)

    base = path.sub(/#{SEPARATOR}+\z/, '') # strip all trailing separators
             .split(SEPARATOR)[-1]

    if base.nil? or base.empty?
      return SEPARATOR.dup if path.start_with?(SEPARATOR)
      return ""
    end
    
    # Optional file-extension removal, which accepts a file-glob like extension
    # which is converted into a regex
    if ext
      raise TypeError, "ext must be a string" unless ext.is_a?(String)
      if ext == ".*"
        base = base.sub(/\.[^.]+\z/, '')
      elsif base.end_with?(ext)
        base = base[0..(-1*(ext.size+1))]
      end
    end
    base.dup
  end

  def self.dirname(path, depth=1)
    if path.respond_to?(:to_path)
      path = path.to_path
    end
    if path.respond_to?(:to_str)
      path = path.to_str
    end
    raise TypeError, "path must be convertable to String" unless path.is_a?(String)
    depth = depth.to_int if !depth.is_a?(Integer) && depth.respond_to?(:to_int)
    if depth < 0
      raise ArgumentError, "negative level: #{depth}"
    elsif depth == 0
      return path
    end
    tokens = path.sub(/#{SEPARATOR}+\z/, '') # strip all trailing separators
               .split(SEPARATOR) # then split on separator
    depth.times do 
      tokens.pop # remove file/dirs from the end
      # effectively remove any additional trailing separators.
      while tokens[-1] == ""
        tokens.pop
      end
    end
    dir = tokens.join(SEPARATOR)
    return SEPARATOR if dir.empty? && path.start_with?(SEPARATOR)
    return '.' if dir.empty?
    dir
  end

  def self.extname(path)
    file = File.basename(path)
    return "" unless file.include?(".")  # no ext if no period
    return "" if file =~ /^\.+\z/ # no ext if all periods
    return "" if file.start_with?(".") &&  !file[1..].include?(".")
    return "." if file.end_with?(".") # ext is a period if ends with period
    file.sub(/^.*\./,'.')
  end

  # NOTE: This method is implemented with Regexps, and that was most certainly a mistake.
  # If you feel the need to fix a bug here, I'm sorry. We should really rewrite this
  # with a proper tokenizer and state machine.
  def self.fnmatch(pattern, path, flags = 0)
    unless pattern.is_a?(String)
      raise TypeError, "no implicit conversion of #{pattern.class} into String"
    end

    path = path.to_path if !path.is_a?(String) && path.respond_to?(:to_path)
    unless path.is_a?(String)
      raise TypeError, "no implicit conversion of #{path.class} into String"
    end

    flags = flags.to_int if !flags.is_a?(Integer) && flags.respond_to?(:to_int)
    unless flags.is_a?(Integer)
      raise TypeError, "no implicit conversion of #{flags.class} into Integer"
    end

    if flags & FNM_CASEFOLD != 0
      pattern = pattern.upcase
      path = path.upcase
    end

    pattern = pattern.gsub(/\.|\(|\)|\||(?<!\[)\^|\$|!|\+/) { |m| '\\' + m }

    if flags & FNM_EXTGLOB != 0
      pattern = pattern.gsub(/\{([^}]*)\}/) do |m|
        # FIXME: $1 not set here
        "(#{m[1...-1].tr(',', '|')})"
      end
    end

    if flags & FNM_PATHNAME != 0
      return true if path == ".#{SEPARATOR}" && pattern.start_with?("\\.#{SEPARATOR}**")

      if flags & FNM_DOTMATCH == 0
        return false if path == '.' && pattern != '\.**' && pattern != '\.*'

        file = path.match(/(?<=#{SEPARATOR})[^#{SEPARATOR}]*\z/)&.to_s || path
        dir = path.slice(0, path.size - file.size)
        file_pattern = pattern.match(/(?<=#{SEPARATOR})[^#{SEPARATOR}]*\z/)&.to_s || pattern
        dir_pattern = pattern.slice(0, pattern.size - file_pattern.size)
        #p(path: path, pattern: pattern, dir: dir, file: file, dir_pattern: dir_pattern, file_pattern: file_pattern)
        if dir =~ /^\.[^#{SEPARATOR}]|#{SEPARATOR}\./ && dir_pattern !~ /^\\\.|#{SEPARATOR}\\\./
          #p 1
          # directory part of pattern does not allow hidden directories
          return false
        end
        if file =~ /^\.|#{SEPARATOR}\./ && file_pattern !~ /^\\\.|#{SEPARATOR}\\\./
          #p 2
          # file part of pattern does not allow hidden files
          return false
        end
      end

      pattern = pattern.gsub(/\*?\*(#{SEPARATOR}?)/) do |m|
        if m == "**#{SEPARATOR}"
          "(.*#{SEPARATOR}|^)"
        else
          "[^#{SEPARATOR}]*#{m[1]}"
        end
      end
      pattern = pattern.gsub(/\?/, "[^#{SEPARATOR}]")
                       .gsub(/\[((?!\^)[^\]]*)#{SEPARATOR}([^\]]*)\]/, '[\1\2]')
    else
      return false if flags & FNM_DOTMATCH == 0 && path.start_with?('.') && !pattern.start_with?('\.')
      pattern = pattern.gsub(/\*?\*/, '.*')
                       .gsub(/\?/, '.')
    end

    return false if pattern.include?('[]')

    pattern = pattern.gsub(/\[\\!/, '[^')
    #p(pattern: pattern, path: path)

    Regexp.new('\A' + pattern + '\z').match?(path)
  end

  class << self
    alias fnmatch? fnmatch
  end
end
