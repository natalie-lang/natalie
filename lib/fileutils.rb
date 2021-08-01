module FileUtils
  def self.rm_rf(path)
    if File.directory?(path)
      Dir.each_child(path) do |entry|
        entry_path = File.join(path, entry)
        if File.file?(entry_path)
          File.unlink(entry_path)
        elsif File.directory?(entry_path)
          rm_rf(entry_path)
        end
      end
      Dir.rmdir(path)
    elsif File.file?(path)
      File.unlink(path)
    end
  end
end
