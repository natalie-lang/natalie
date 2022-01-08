def with_each_token(text)
  l = 0
  copy = ''
  prev_token = nil
  text.length.times do |r|
    c = text[r]
    is_last = r == (text.length - 1)
    if /\s/.match?(c) || is_last
      token = text[is_last ? l..r : l...r]

      maybe_highlighted_token = yield(token, l, r, is_last) || token

      is_last ? copy.concat(maybe_highlighted_token) : copy.concat(maybe_highlighted_token, c)

      prev_token = token
      l = r + 1
    end
  end
  copy
end
