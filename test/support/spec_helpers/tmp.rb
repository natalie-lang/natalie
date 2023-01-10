=begin
Copyright (c) 2008 Engine Yard, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
=end

# Creates a temporary directory in the current working directory
# for temporary files created while running the specs. All specs
# should clean up any temporary files created so that the temp
# directory is empty when the process exits.

SPEC_TEMP_DIR_PID = Process.pid

if spec_temp_dir = ENV["SPEC_TEMP_DIR"]
  spec_temp_dir = File.realdirpath(spec_temp_dir)
else
  spec_temp_dir = "#{File.realpath(Dir.pwd)}/rubyspec_temp/#{SPEC_TEMP_DIR_PID}"
end
SPEC_TEMP_DIR = spec_temp_dir

SPEC_TEMP_UNIQUIFIER = "0"

at_exit do
  begin
    if SPEC_TEMP_DIR_PID == Process.pid
      Dir.delete SPEC_TEMP_DIR if File.directory? SPEC_TEMP_DIR
    end
  rescue SystemCallError
    STDERR.puts <<-EOM

-----------------------------------------------------
The rubyspec temp directory is not empty. Ensure that
all specs are cleaning up temporary files:
  #{SPEC_TEMP_DIR}
-----------------------------------------------------

    EOM
  rescue Object => e
    STDERR.puts "failed to remove spec temp directory #{SPEC_TEMP_DIR}"
    STDERR.puts e.message
  end
end

def tmp(name, uniquify = true)
  mkdir_p SPEC_TEMP_DIR unless Dir.exist? SPEC_TEMP_DIR

  if uniquify and !name.empty?
    # TODO: revert when String#rindex and String#insert are implemented
    #slash = name.rindex "/"
    #index = slash ? slash + 1 : 0
    #name.insert index, "#{SPEC_TEMP_UNIQUIFIER.succ!}-"
    slash = name.reverse.index("/")
    index = slash ? name.size - slash : 0
    name = name[0...index] + "#{SPEC_TEMP_UNIQUIFIER.succ!}-" + name[index..-1]
  end
  File.join SPEC_TEMP_DIR, name
end
