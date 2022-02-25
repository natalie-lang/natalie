module Base64
  def self.encode64(str)
    [str].pack("m")
  end

  def self.strict_encode64(str)
    [str].pack("m0")
  end

  # NATFIXME: Make this spec-compliant.
  def self.urlsafe_encode64(bin, padding: true)
    strict_encode64(bin)
  end
end
