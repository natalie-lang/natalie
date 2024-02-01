require 'natalie/inline'
require 'tempfile.cpp'

class Tempfile
  class << self
    def create(basename)
      if block_given?
        begin
          tmpfile = new(basename)
          yield(tmpfile.instance_variable_get(:@tmpfile))
        ensure
          tmpfile.close!
        end
      else
        new(basename).instance_variable_get(:@tmpfile)
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

  def unlink
    File.unlink(@tmpfile.path)
  end
  alias delete unlink

  (File.public_instance_methods(false) + [:closed?, :open, :pos, :print, :puts, :rewind, :write]).each do |method|
    define_method(method) do |*args, **kwargs, &block|
      @tmpfile.public_send(method, *args, **kwargs, &block)
    end
  end
end
