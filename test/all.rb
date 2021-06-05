require 'fileutils'
require 'minitest/reporters'
Minitest::Reporters.use!(Minitest::Reporters::ProgressReporter.new(detailed_skip: false))

FileUtils.mkdir_p(File.expand_path('tmp', __dir__))

Dir[File.expand_path('ruby/*_test.rb', __dir__)].each do |path|
  load(path)
end
