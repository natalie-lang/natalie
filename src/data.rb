class Data
  def self.define(*members)
    members = members.map(&:to_sym)

    Class.new do
      define_singleton_method(:members) { members }
    end
  end
end
