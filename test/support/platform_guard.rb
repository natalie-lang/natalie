class PlatformGuard
  POINTER_SIZE = 0.size * 8

  # We'll add a real implementation the moment rubyspecs gets Natalie-specific specs
  def self.implementation?(*)
    false
  end

  def self.windows?
    false
  end
end
