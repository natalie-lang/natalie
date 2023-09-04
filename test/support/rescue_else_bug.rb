l = -> {
  begin
    raise 'foo'
  rescue => e
    # trigger LocalJumpError that is not used
    nil.tap { return :foo if false }
    'good'
  else
    # this code should not run
    'bad'
  end
}

puts l.call
