# skip-ruby

require_relative '../spec_helper'

describe 'caller' do
  ROOT = File.expand_path('../..', __dir__)

  def get_caller_for_real
    caller
  end

  def get_caller
    get_caller_for_real
  end

  def make_path_relative(line)
    if line.start_with?('/')
      line[(ROOT.size + 1)..]
    else
      line
    end
  end

  def normalize_lines(lines)
    lines.reject { |line| line =~ %r{test/support} }.map { |line| make_path_relative(line) }
  end

  it 'returns an array of paths, line numbers, and call site names' do
    # Natalie spells nested block labels as repeated "block in" prefixes.
    normalize_lines(get_caller).should ==
      ["test/natalie/caller_test.rb:13:in 'get_caller'", "test/natalie/caller_test.rb:30:in 'block in block in <main>'"]
  end
end
