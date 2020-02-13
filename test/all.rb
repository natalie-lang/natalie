require 'fileutils'

FileUtils.mkdir_p(File.expand_path('tmp', __dir__))

Dir['test/ruby/*_test.rb'].each do |path|
  load(path)
end
