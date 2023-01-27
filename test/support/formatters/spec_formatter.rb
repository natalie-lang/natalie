class SpecFormatter < DefaultFormatter

  def print_context(cxt)
    puts cxt.map(&:description).join(" ")
  end

  def print_success(_cxt, test)
    puts "- [OK]   #{test}"
  end

  def print_error(_cxt, test, _e)
    puts "- [Err]  #{test}"
  end

  def print_failure(_cxt, test, _e)
    puts "- [Fail] #{test}"
  end

  def print_skipped(_cxt, test)
    puts "- [Skip] #{test}"
  end

end
