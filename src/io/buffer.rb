class IO
  class Buffer
    class AccessError < RuntimeError
    end

    class AllocationError < RuntimeError
    end

    class InvalidatedError < RuntimeError
    end

    class LockedError < RuntimeError
    end

    class MaskError < ArgumentError
    end
  end
end
