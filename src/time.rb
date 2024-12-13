class Time
  include Comparable

  class << self
    alias gm utc
    alias mktime local
  end

  alias ctime asctime
  alias day mday
  alias gmt? utc?
  alias mon month
  alias tv_nsec nsec
  alias tv_sec to_i
  alias tv_usec usec

  def sunday?
    wday == 0
  end

  def monday?
    wday == 1
  end

  def tuesday?
    wday == 2
  end

  def wednesday?
    wday == 3
  end

  def thursday?
    wday == 4
  end

  def friday?
    wday == 5
  end

  def saturday?
    wday == 6
  end

  def _dump(depth = -1)
    raise NotImplementedError, 'Implement Time#_dump'
  end
end
