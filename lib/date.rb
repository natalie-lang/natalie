require 'natalie/inline'

class Date
  include Comparable

  ITALY = 2_299_161 # 1582-10-15
  ENGLAND = 2_361_222 # 1752-09-14

  freeze = ->(values) { values.map { |value| value.nil? ? nil : value.freeze }.freeze }

  ABBR_DAYNAMES = freeze[%w[Sun Mon Tue Wed Thu Fri Sat]]
  ABBR_MONTHNAMES = freeze[[nil, 'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']]
  DAYNAMES = freeze[%w[Sunday Monday Tuesday Wednesday Thursday Friday Saturday]]
  MONTHNAMES = freeze[[nil, 'January', 'February', 'March', 'April', 'May', 'June',
                       'July', 'August', 'September', 'October', 'November', 'December']]

  Error = Class.new(ArgumentError)

  class << self
    def civil_to_jd(y, m, d, start)
      if m <= 2
        y -= 1
        m += 12
      end
      a = (y / 100.0).floor
      b = 2 - a + (a / 4.0).floor
      n = (365.25 * (y + 4716)).floor + (30.6001 * (m + 1)).floor + d + b - 1524
      n < start ? n - b : n
    end

    def gregorian_leap?(year)
      (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)
    end

    def iso8601(string = '-4712-01-01')
      date = _iso8601(string)
      raise Error, 'invalid date' if date.empty?
      new(date[:year], date[:mon], date[:mday])
    end

    def _iso8601(string)
      return {} if string.nil?
      string = string.to_str if !string.is_a?(String) && string.respond_to?(:to_str)
      unless string.is_a?(String)
        raise TypeError, "no implicit conversion of #{string.nil? ? 'nil' : string.class} into String"
      end

      if string =~ /\A(\d{4})(\d\d)(\d\d)\z/ || string =~ /\A(-?\d{4})-(\d\d)-(\d\d)\z/
        return { mday: $3.to_i, mon: $2.to_i, year: $1.to_i }
      end
      {}
    end

    def jd(number = 0, start = Date::ITALY)
      year, month, mday = jd_to_civil(number, start)
      date = allocate
      date.instance_variable_set(:@year, year)
      date.instance_variable_set(:@month, month)
      date.instance_variable_set(:@mday, mday)
      date.instance_variable_set(:@start, start)
      date.instance_variable_set(:@jd, number)
      date
    end

    def jd_to_civil(number = 0, start = Date::ITALY)
      if number < start
        a = number
      else
        x = ((number - 1_867_216.25) / 36_524.25).floor
        a = number + 1 + x - (x / 4.0).floor
      end
      b = a + 1524
      c = ((b - 122.1) / 365.25).floor
      d = (365.25 * c).floor
      e = ((b - d) / 30.6001).floor
      mday = b - d - (30.6001 * e).floor
      if e <= 13
        month = e - 1
        year = c - 4716
      else
        month = e - 13
        year = c - 4715
      end
      [year, month, mday]
    end

    def julian_leap?(year)
      year % 4 == 0
    end

    def parse(string = '-4712-01-01', comp = true)
      year = comp ? 2000 : 0
      string = string.to_str if !string.is_a?(String) && string.respond_to?(:to_str)
      unless string.is_a?(String)
        raise TypeError, "no implicit conversion of #{string.nil? ? 'nil' : string.class} into String"
      end

      case string
      when /\A(\d\d)\z/ # DD
        new(today.year, today.month, $1.to_i)
      when /\A(\d\d)(\d\d)\z/ # MMDD
        new(today.year, $1.to_i, $2.to_i)
      when /\A(\d\d)(\d\d)(\d\d)\z/ # YYMMDD
        year = $1.to_i
        year = year <= 68 ? year + 2000 : year + 1900
        new(year, $2.to_i, $3.to_i)
      when /\A(\d{4})(\d\d)(\d\d)\z/ # YYYYMMDD
        new($1.to_i, $2.to_i, $3.to_i)
      when %r{\A(\d{4})/(\d\d)/(\d\d)\z} # YYYY/MM/DD
        new($1.to_i, $2.to_i, $3.to_i)
      when %r{\A(\d\d)/(\d\d)/(\d{4})\z} # DD/MM/YYYY
        new($3.to_i, $2.to_i, $1.to_i)
      when %r{\A(\d\d)/(\d\d)/(\d\d)\z}  # YY/MM/DD
        new(year + $1.to_i, $2.to_i, $3.to_i)
      when /\A(\d{4})-(\d\d)-(\d\d)\z/ # YYYY-MM-DD
        new($1.to_i, $2.to_i, $3.to_i)
      when /\A(\d\d)-(\d\d)-(\d{4})\z/ # MM-DD-YYYY
        new($3.to_i, $2.to_i, $1.to_i)
      when /\A(\d\d)-(\d\d)-(\d\d)\z/ #  YY-MM-DD
        new(year + $1.to_i, $2.to_i, $3.to_i)
      when /\A(\d{4})\.(\d\d)\.(\d\d)\z/ # YYYY.MM.DD
        new($1.to_i, $2.to_i, $3.to_i)
      when /\A(\d\d)\.(\d\d)\.(\d{4})\z/ # DD.MM.YYYY
        new($3.to_i, $2.to_i, $1.to_i)
      when /\A(\d\d)\.(\d\d)\.(\d\d)\z/ # YY.MM.DD
        new(year + $1.to_i, $2.to_i, $3.to_i)
      else
        raise Date::Error, 'invalid date'
      end
    end

    def today(start = Date::ITALY)
      time = Time.now
      new(time.year, time.month, time.mday, start)
    end

    alias civil new
    alias leap? gregorian_leap?
  end

  def initialize(year = -4712, month = 1, mday = 1, start = Date::ITALY)
    if month < 0
      month += 13
    end
    unless month.between?(1, 12)
      raise Date::Error, 'invalid date'
    end
    last = last_mday(year, month)
    if mday < 0
      mday = last + mday + 1
    end
    if mday < 1 || mday > last
      raise Date::Error, 'invalid date'
    end
    @year = year
    @month = month
    @mday = mday
    @start = start
    @jd = self.class.civil_to_jd(year, month, mday, start)
    if @jd >= start
      _y, m, d = self.class.jd_to_civil(@jd)
      unless m == @month && d == @mday
        raise Date::Error, 'invalid date'
      end
    end
  end

  def +(other)
    if other.is_a?(Numeric)
      self.class.jd(@jd + other, @start)
    else
      raise TypeError, 'expected numeric'
    end
  end

  def -(other)
    if other.is_a?(self.class)
      Rational(@jd - other.jd)
    elsif other.is_a?(Numeric)
      self.class.jd(@jd - other, @start)
    else
      raise TypeError, 'expected numeric'
    end
  end

  def <<(n)
    unless n.is_a?(Numeric)
      raise TypeError, 'expected numeric'
    end
    self >> (-n)
  end

  def <=>(other)
    if other.is_a?(self.class)
      @jd <=> other.jd
    elsif other.is_a?(Numeric)
      @jd <=> other
    end
  end

  def >>(other)
    unless other.is_a?(Numeric)
      raise TypeError, "#{other.class} can't be coerced into Integer"
    end
    other = other.to_int
    i = (@year * 12) + (@month - 1) + other
    year = i.div(12)
    month = (i % 12) + 1
    mday = @mday
    last = last_mday(year, month)
    if mday > last
      mday = last
    end
    self.class.new(year, month, mday)
  rescue Date::Error
    self.class.jd(@start - 1)
  end

  def asctime
    strftime('%a %b %e %H:%M:%S %Y')
  end

  def downto(min)
    return to_enum(:downto, min) unless block_given?
    date = self
    while date >= min
      yield date
      date -= 1
    end
  end

  def eql?(other)
    unless other.is_a?(self.class)
      return false
    end
    (self <=> other).zero?
  end

  def friday?
    wday == 5
  end

  def gregorian?
    @jd >= @start
  end

  def inspect
    "#<Date: #{self} ((#{@jd}j),#{@start}j)>"
  end

  attr_reader :jd

  def julian?
    @jd < @start
  end

  def leap?
    self.class.gregorian_leap?(@year)
  end

  attr_reader :mday

  def monday?
    wday == 1
  end

  attr_reader :month

  def next
    self.class.jd(@jd + 1, @start)
  end

  def next_day(n = 1)
    self + n
  end

  def next_month(n = 1)
    self >> n
  end

  def next_year(n = 1)
    self >> (n * 12)
  end

  def prev_day(n = 1)
    self - n
  end

  def prev_month(n = 1)
    self << n
  end

  def prev_year(n = 1)
    self << (n * 12)
  end

  def rfc2822
    strftime('%a, %-d %b %Y %T %z')
  end

  def rfc3339
    strftime('%Y-%m-%dT%H:%M:%S+00:00')
  end

  def saturday?
    wday == 6
  end

  def start
    @start.to_f
  end

  def step(limit, step = 1)
    return to_enum(:step, limit, step) unless block_given?
    date = self
    if step < 0
      while date >= limit
        yield date
        date += step
      end
    elsif step > 0
      while date <= limit
        yield date
        date += step
      end
    end
  end

  def strftime(format = '%F') # rubocop:disable Lint/UnusedMethodArgument
    __inline__ <<-END
      format_var->assert_type(env, Object::Type::String, "String");
      struct tm time = { 0 };
      time.tm_year = IntegerObject::convert_to_int(env, self->ivar_get(env, "@year"_s)) - 1900;
      time.tm_mon = IntegerObject::convert_to_int(env, self->ivar_get(env, "@month"_s)) - 1;
      time.tm_mday = IntegerObject::convert_to_int(env, self->ivar_get(env, "@mday"_s));
      time.tm_gmtoff = 0;
      time.tm_isdst = 0;
      int maxsize = 32;
      char buffer[maxsize];
      auto length = ::strftime(buffer, maxsize, format_var->as_string()->c_str(), &time);
      return new StringObject { buffer, length, Encoding::US_ASCII };
    END
  end

  def sunday?
    wday == 0
  end

  def thursday?
    wday == 4
  end

  def to_date
    self
  end

  def to_s
    strftime('%Y-%m-%d')
  end

  def to_time
    Time.new(@year, @month, @mday)
  end

  def tuesday?
    wday == 2
  end

  def upto(max)
    return to_enum(:upto, max) unless block_given?
    date = self
    while date <= max
      yield date
      date += 1
    end
  end

  def wday
    (@jd + 1) % 7
  end

  def wednesday?
    wday == 3
  end

  attr_reader :year

  alias ctime asctime
  alias day mday
  alias iso8601 to_s
  alias mon month
  alias rfc822 rfc2822
  alias succ next
  alias xmlschema to_s

  private

  MONTHDAYS = [nil, 31, nil, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31].freeze

  def last_mday(year, month)
    if month == 2
      self.class.leap?(year) ? 29 : 28
    else
      MONTHDAYS[month]
    end
  end
end

class Time
  def to_date
    Date.new(year, month, mday)
  end
end
