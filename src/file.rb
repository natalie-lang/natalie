class File
  def self.join(*parts)
    parts.join('/')
  end

  def self.dirname(path)
    dir = path.sub(%r{/\z}, '').split('/')[0..-2].join('/')
    return '/' if dir.empty? && path.start_with?('/')
    return '.' if dir.empty?
    dir
  end
end
