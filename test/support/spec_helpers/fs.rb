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

# Copies a file
def cp(source, dest)
  IO.copy_stream source, dest
end

# Creates each directory in path that does not exist.
def mkdir_p(path)
  parts = File.expand_path(path).split %r[/|\\]
  name = parts.shift
  parts.each do |part|
    name = File.join name, part

    if File.file? name
      raise ArgumentError, "path component of #{path} is a file"
    end

    unless File.directory? name
      begin
        Dir.mkdir name
      rescue Errno::EEXIST => e
        if File.directory? name
          # OK, another process/thread created the same directory
        else
          raise e
        end
      end
    end
  end
end

# Recursively removes all files and directories in +path+
# if +path+ is a directory. Removes the file if +path+ is
# a file.
def rm_r(*paths)
  paths.each do |path|
    path = File.expand_path path
    #prefix = SPEC_TEMP_DIR # from mspec, need to revert when possible
    prefix = "#{Dir.pwd}/tmp" 
    unless path[0, prefix.size] == prefix
      raise ArgumentError, "#{path} is not prefixed by #{prefix}"
    end

    # File.symlink? needs to be checked first as
    # File.exist? returns false for dangling symlinks
    if File.symlink? path
      File.unlink path
    elsif File.directory? path
      `rm -rf #{path}`
      #Dir.entries(path).each { |x| rm_r "#{path}/#{x}" unless x =~ /^\.\.?$/ }
      #Dir.rmdir path
    elsif File.exist? path
      File.delete path
    end
  end
end

# Creates a file +name+. Creates the directory for +name+
# if it does not exist.
def touch(name, mode = "w")
  mkdir_p File.dirname(name)
  has_block = block_given? # NATFIXME: workaround for issue #742
  File.open(name, mode) do |f|
    yield f if has_block
  end
end
