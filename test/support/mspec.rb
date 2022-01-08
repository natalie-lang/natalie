class MSpec
  def self.features
    @features || {}
  end

  def self.enable_feature(name)
    @features ||= {}
    @features[name] = true
  end
end
