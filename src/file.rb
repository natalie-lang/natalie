class File
  SEPARATOR = '/'.freeze

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
      parts.join(SEPARATOR)
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
    if depth < 0
      raise ArgumentError, "depth cannot be negative"
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
end
