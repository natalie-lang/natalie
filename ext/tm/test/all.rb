require 'fileutils'
require 'minitest/reporters'

case ENV['REPORTER']
when 'spec'
  Minitest::Reporters.use!(Minitest::Reporters::SpecReporter.new)
when 'progress', nil
  Minitest::Reporters.use!(Minitest::Reporters::ProgressReporter.new(detailed_skip: false))
end

Dir[File.expand_path('*_test.rb', __dir__)].each { |path| load(path) }
