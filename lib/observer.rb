module Observable
  def changed(state = true)
    @observer_state = state
  end

  def add_observer(observer, callback = :update)
    @observer_peers = {} unless defined? @observer_peers

    unless observer.respond_to? callback
      raise NoMethodError, "observer does not respond to `#{callback}`"
    end

    @observer_peers[observer] = callback
  end

  def notify_observers(*arg)
    if defined? @observer_state and @observer_state
      if defined? @observer_peers
        @observer_peers.each do |observer, callback|
          observer.__send__(callback, *arg)
        end
      end
      
      @observer_state = false
    end
  end
end
