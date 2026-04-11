class String
  alias replace initialize_copy
  alias slice []

  def unicode_normalize(form = :nfc)
    require 'unicode_normalize/normalize'
    UnicodeNormalize.normalize(self, form)
  end

  def unicode_normalize!(form = :nfc)
    require 'unicode_normalize/normalize'
    replace(UnicodeNormalize.normalize(self, form))
  end

  def unicode_normalized?(form = :nfc)
    require 'unicode_normalize/normalize'
    UnicodeNormalize.normalized?(self, form)
  end

  def %(args)
    positional_args =
      if args.respond_to?(:to_ary)
        if (ary = args.to_ary).nil?
          [args]
        elsif ary.is_a?(Array)
          ary
        else
          raise TypeError,
                "can't convert #{args.class.name} to Array (#{args.class.name}#to_ary gives #{ary.class.name})"
        end
      else
        [args]
      end
    Kernel.sprintf(self, *positional_args)
  end
end
