require 'fileutils'
require 'minitest/reporters'

case ENV['REPORTER']
when 'spec'
  Minitest::Reporters.use!(Minitest::Reporters::SpecReporter.new)
when 'progress', nil
  Minitest::Reporters.use!(Minitest::Reporters::ProgressReporter.new(detailed_skip: false))
end

FileUtils.mkdir_p(File.expand_path('tmp', __dir__))

Dir[File.expand_path('ruby/*_test.rb', __dir__)].each { |path| load(path) }
