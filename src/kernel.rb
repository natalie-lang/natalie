module Kernel
  def enum_for(method, *args)
    Enumerator.new do |y|
      send(method, *args) do |*items|
        if items.size > 1
          y << items
        else
          y << items.first
        end
        self
      end
    end
  end
end
