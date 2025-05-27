class Data
  def self.define(*members, &block)
    members = members.map(&:to_sym)

    Class.new(Data) do
      members.each { |name| define_method(name) { instance_variable_get(:"@#{name}") } }

      define_method(:initialize) do |*args, **kwargs|
        if args.empty? && !kwargs.empty?
          kwargs = kwargs.transform_keys(&:to_sym)
          missing = members - kwargs.keys
          unless missing.empty?
            raise ArgumentError, "missing keyword#{missing.size == 1 ? '' : 's'}: #{missing.map(&:inspect).join(', ')}"
          end
          extra = kwargs.keys - members
          unless extra.empty?
            raise ArgumentError, "unknown keyword#{extra.size == 1 ? '' : 's'}: #{extra.map(&:inspect).join(', ')}"
          end
          kwargs.each { |name, value| instance_variable_set(:"@#{name}", value) }
        elsif args.empty?
          raise ArgumentError, "missing keyword#{members.size == 1 ? '' : 's'}: #{members.map(&:inspect).join(', ')}"
        else
          if members.size != args.size
            raise ArgumentError, "wrong number of arguments (given #{args.size}, expected #{members.size})"
          end

          members.zip(args) { |name, value| instance_variable_set(:"@#{name}", value) }
        end
      end

      define_method(:inspect) do
        name = if self.class.to_s.start_with?('#')
                 ''
               else
                 " #{self.class}"
               end
        if !Fiber[:__data_inspect_current]
          Fiber.new(storage: { __data_inspect_current: [] }, &->() { inspect }).resume
        elsif Fiber[:__data_inspect_current].include?(object_id)
          "#<data#{name}:...>"
        else
          Fiber[:__data_inspect_current] << object_id
          name = if self.class.to_s.start_with?('#')
                   ''
                 else
                   " #{self.class}"
                 end
          "#<data#{name}#{members.map { |member| " #{member}=#{public_send(member).inspect}" }.join(',')}>"
        end
      end
      alias_method :to_s, :inspect

      define_method(:to_h) do |&block|
        if block
          members.to_h { |member| block.call(member, public_send(member)) }
        else
          members.to_h { |member| [member, public_send(member)] }
        end
      end

      define_method(:with) do |**kwargs|
        if kwargs.empty?
          self
        else
          self.class.new(**to_h.merge(kwargs.transform_keys(&:to_sym)))
        end
      end

      define_method :== do |other|
        self.class == other.class && to_h == other.to_h
      end

      define_method(:deconstruct) do
        members.map { public_send(it) }
      end

      define_method(:deconstruct_keys) do |keys|
        raise TypeError, "wrong argument type #{keys.class} (expected Array or nil)" if !keys.nil? && !keys.is_a?(Array)
        keys = members if keys.nil?
        next {} if keys.size > members.size
        result = {}
        keys.each do |key|
          key_sym = if key.is_a?(Symbol)
                      key
                    elsif key.is_a?(String)
                      key.to_sym
                    elsif key.is_a?(Integer)
                      members.fetch(key, nil)
                    elsif key.respond_to?(:to_int)
                      int_key = key.to_int
                      raise TypeError, "can't convert #{key.class} to Integer (#{key.class}#to_int gives #{int_key.class})" unless int_key.is_a?(Integer)
                      members.fetch(int_key, nil)
                    else
                      raise TypeError, "no implicit conversion of #{key.class} into Integer"
                    end
          break unless members.include?(key_sym)
          result[key] = public_send(key_sym)
        end
        result
      end

      define_method(:hash) do
        super ^ self.class.hash
      end

      define_singleton_method(:[]) { |*args, **kwargs| new(*args, **kwargs) }

      define_singleton_method(:members) { members.dup }
      define_method(:members) { members.dup }

      instance_eval(&block) if block
    end
  end
end
