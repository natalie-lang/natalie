module Observable
  def changed(state = true)
    @observer_state = state
  end

  def add_observer(observer, callback = :update)
    @observers = {} unless defined? @observers

    unless observer.respond_to? callback
      raise NoMethodError, "observer does not respond to `#{callback}`"
    end

    @observers[observer] = callback
  end

  def delete_observer(observer)
    @observers.delete observer if defined? @observers
  end

  def delete_observers
    @observers.clear if defined? @observers
  end

  def count_observers
    if defined? @observers
      @observers.size
    else
      0
    end
  end

  def notify_observers(*arg)
    if defined?(@observer_state) && @observer_state
      if defined?(@observers)
        @observers.each do |observer, callback|
          observer.__send__(callback, *arg)
        end
      end

      @observer_state = false
    end
  end
end
