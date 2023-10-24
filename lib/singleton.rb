module Singleton
  module ClassMethods
    def instance
      @instance ||= new
    end

    def _load(arg)
      instance
    end
  end

  def self.included(base)
    base.private_class_method :new, :allocate
    base.extend ClassMethods
  end

  def clone
    raise TypeError, "can't clone instance of singleton #{self.class.name}"
  end

  def dup
    raise TypeError, "can't dup instance of singleton #{self.class.name}"
  end

  def _dump(depth = -1)
    ''
  end
end
