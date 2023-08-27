class YamlFormatter
  def print_context(*) ; end

  def print_success(*); end

  def print_error(*); end

  def print_failure(*); end

  def print_skipped(*); end

  def print_finish(test_count, failures, errors, skipped)
    print "---\n"
    if failures.any? || errors.any?
      print "exceptions:\n"
      (errors + failures).each do |failure|
        context, test, error = failure
        type = error.is_a?(SpecFailedException) ? 'FAILED' : 'ERROR'
        puts "- type: #{type}"
        puts "  class: #{error.class.name.inspect}"
        puts "  message: #{error.message.inspect}"
        puts "  backtrace: "
        error.backtrace.each do |line|
          puts "  - #{line.inspect}"
        end
      end
    end

    print 'examples: ', test_count, "\n"
    print 'failures: ', failures.size, "\n"
    print 'errors: ', errors.size, "\n"
  end
end
