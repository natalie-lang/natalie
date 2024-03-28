require 'yaml'

class YamlFormatter
  def print_context(*) ; end

  def print_success(*); end

  def print_error(*); end

  def print_failure(*); end

  def print_skipped(*); end

  def print_finish(test_count, failures, errors, skipped)
    struct = {
      'examples' => test_count,
      'failures' => failures.size,
      'errors' =>  errors.size,
    }
    if failures.any? || errors.any?
      outcomes = (errors + failures).map do |failure|
        context, test, error = failure
        outcome = error.is_a?(SpecFailedException) ? 'FAILED' : 'ERROR'
        str = "#{test} #{outcome}\n"
        str << error.message << "\n" << error.backtrace.to_s
      end
      struct['exception'] = outcomes
    end
    print struct.to_yaml
  end
end
