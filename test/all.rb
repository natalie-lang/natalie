require 'fileutils'

FileUtils.mkdir_p(File.expand_path('tmp', __dir__))

Dir[File.expand_path('ruby/*_test.rb', __dir__)].each do |path|
  load(path)
end
