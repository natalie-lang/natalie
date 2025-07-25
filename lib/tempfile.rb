require 'natalie/inline'
require 'tempfile.cpp'
require 'tmpdir'

class Tempfile
  class << self
    def create(*args, **kwargs, &block)
      if block_given?
        begin
          tmpfile = new(*args, **kwargs, &block)
          yield(tmpfile.instance_variable_get(:@tmpfile))
        ensure
          tmpfile.close!
        end
      else
        new(*args, **kwargs, &block).instance_variable_get(:@tmpfile)
      end
    end

    def open(*args, **kwargs)
      if block_given?
        begin
          tempfile = new(*args, **kwargs)
          yield(tempfile)
        ensure
          tempfile.close
        end
      else
        new(*args, **kwargs)
      end
    end
  end

  __bind_method__ :initialize, :Tempfile_initialize

  def close(unlink_now = false)
    @tmpfile.close
    unlink if unlink_now
  end

  def close!
    close(true)
  end

  def _close
    @tmpfile.close
  end
  protected :_close

  def path
    return nil unless File.exist?(@tmpfile.path)
    @tmpfile.path
  end

  def size
    path = self.path
    if path
      File.size(path)
    elsif !closed?
      @tmpfile.size
    else
      raise Errno::ENOENT, 'No such file or directory'
    end
  end
  alias length size

  def unlink
    File.unlink(@tmpfile.path) if File.exist?(@tmpfile.path)
  end
  alias delete unlink

  (File.public_instance_methods - public_instance_methods).each do |method|
    define_method(method) { |*args, **kwargs, &block| @tmpfile.public_send(method, *args, **kwargs, &block) }
  end
end
