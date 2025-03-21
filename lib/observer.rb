module Observable
  def changed(state = true)
    @observer_state = state
  end

  def add_observer(observer, callback = :update)
    @observers = {} unless defined?(@observers)

    raise NoMethodError, "observer does not respond to `#{callback}`" unless observer.respond_to? callback

    @observers[observer] = callback
  end

  def delete_observer(observer)
    @observers.delete observer if defined?(@observers)
  end

  def delete_observers
    @observers.clear if defined?(@observers)
  end

  def count_observers
    if defined?(@observers)
      @observers.size
    else
      0
    end
  end

  def notify_observers(*arg)
    if defined?(@observer_state) && @observer_state
      @observers.each { |observer, callback| observer.__send__(callback, *arg) } if defined?(@observers)

      @observer_state = false
    end
  end
end
