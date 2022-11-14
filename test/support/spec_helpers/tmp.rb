# Creates a temporary directory in the current working directory
# for temporary files created while running the specs. All specs
# should clean up any temporary files created so that the temp
# directory is empty when the process exits.

SPEC_TEMP_DIR_PID = Process.pid

if spec_temp_dir = ENV["SPEC_TEMP_DIR"]
  spec_temp_dir = File.realdirpath(spec_temp_dir)
else
  # TODO: revert when File.realpath is implemented
  #spec_temp_dir = "#{File.realpath(Dir.pwd)}/rubyspec_temp/#{SPEC_TEMP_DIR_PID}"
  spec_temp_dir = "./tmp.#{SPEC_TEMP_DIR_PID}"
end
SPEC_TEMP_DIR = spec_temp_dir

SPEC_TEMP_UNIQUIFIER = "0"

at_exit do
  begin
    if SPEC_TEMP_DIR_PID == Process.pid
      # TODO: revert when Dir.delete is implemented
      #Dir.delete SPEC_TEMP_DIR if File.directory? SPEC_TEMP_DIR
      `rm -r #{SPEC_TEMP_DIR}` if File.directory? SPEC_TEMP_DIR
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
    index = slash ? name.size - slash - 1 : 0
    name = name[0...index] + "#{SPEC_TEMP_UNIQUIFIER.succ!}-" + name[index..-1]
  end

  File.join SPEC_TEMP_DIR, name
end
