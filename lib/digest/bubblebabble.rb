require 'digest'

module Digest
  # http://web.mit.edu/kenta/www/one/bubblebabble/spec/jrtrjwzi/draft-huima-01.txt
  def self.bubblebabble(digest)
    vowels = ['a', 'e', 'i', 'o', 'u', 'y'].freeze
    consonants = [
      'b', 'c', 'd', 'f', 'g', 'h', 'k', 'l', 'm', 'n',
      'p', 'r', 's', 't', 'v', 'z', 'x'
    ].freeze

    digest = digest.to_str if !digest.is_a?(String) && digest.respond_to?(:to_str)
    raise TypeError, "no implicit conversion of #{digest.class} into String" unless digest.is_a?(String)

    i = 0
    seed = 1
    str = 'x'
    loop do
      if i >= digest.size
        str << vowels[seed % 6]
        str << 'x'
        str << vowels[seed / 6]
        break
      end

      byte1 = digest[i].ord
      i += 1
      str << vowels[(((byte1 >> 6) & 3) + seed) % 6]
      str << consonants[(byte1 >> 2) & 15];
      str << vowels[((byte1 & 3) + (seed / 6)) % 6]

      break if i >= digest.size

      byte2 = digest[i].ord
      i += 1
      str << consonants[(byte2 >> 4) & 15];
      str << '-'
      str << consonants[byte2 & 15];

      seed = (seed * 5 + byte1 * 7 + byte2) % 36
    end
    str + 'x'
  end
end
