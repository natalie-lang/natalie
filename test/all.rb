require 'fileutils'
require 'minitest/reporters'

if ENV['PARALLEL']
  require 'minitest/parallel_fork'
  module Minitest
    # HACK: minitest/reporters doesn't seem to play well with parallel_fork :-(
    def self.parallel_fork_stat_reporter(reporter)
      Minitest::SummaryReporter.new
    end
  end
end

case ENV['REPORTER']
when 'spec'
  Minitest::Reporters.use!(Minitest::Reporters::SpecReporter.new)
when 'progress', nil
  Minitest::Reporters.use!(Minitest::Reporters::ProgressReporter.new(detailed_skip: false))
when 'dots'
  # just use the default reporter
end

FileUtils.mkdir_p(File.expand_path('tmp', __dir__))

Dir[File.expand_path('ruby/*_test.rb', __dir__)].each { |path| load(path) }
