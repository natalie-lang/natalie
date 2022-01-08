class DefaultFormatter
  def print_success(*)
    print '.'
  end

  def print_error(*)
    print 'E'
  end

  def print_failure(*)
    print 'F'
  end

  def print_skipped(*)
    print '*'
  end

  def print_finish(test_count, failures, errors, skipped)
    if failures.any? || errors.any?
      puts
      puts
      puts 'Failed specs:'
      (failures + errors).each do |failure|
        context, test, error = failure
        indent = 0
        context.each do |con|
          print ' ' * indent
          puts con.to_s
          indent += 2
        end
        if test
          # nil if using 'specify'
          print ' ' * indent
          puts test
          indent += 2
        end
        print ' ' * indent
        if error.is_a?(SpecFailedException)
          location = nil
          error.backtrace.each do |line|
            if line !~ %r{support\/spec\.rb}
              location = line
              break
            end
          end
          puts error.message
          print ' ' * indent
          puts "(#{location})"
        else
          puts "#{error.message} (#{error.class.name})"
          indent += 2
          error.backtrace.each do |line|
            print ' ' * indent
            puts line
          end
        end
      end
      puts
      puts "#{test_count - failures.size - errors.size} spec(s) passed."
      puts "#{failures.size} spec(s) failed."
      puts "#{errors.size} spec(s) errored."
      puts "#{skipped.size} spec(s) skipped." if skipped.any?
      exit 1
    else
      puts
      puts
      puts "#{test_count} spec(s) passed."
      puts skipped.size.to_s + ' spec(s) skipped.' if skipped.any?
    end
  end
end
