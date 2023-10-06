$LOAD_PATH << File.expand_path('../from_another_file', __dir__)
require 'from_another_file_works'

module LoadPath
  class LongNameWorks
  end
end
