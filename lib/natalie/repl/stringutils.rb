def with_each_token(text)
  l = 0
  copy = ""
  prev_token = nil
  text.length.times do |r|
    c = text[r]
    is_last = r == (text.length - 1)
    if /\s/.match?(c) || is_last
      token = text[if is_last then l..r else l...r end]

      maybe_highlighted_token = yield(token, l, r, is_last) || token

      if is_last
        copy.concat(maybe_highlighted_token)
      else
        copy.concat(maybe_highlighted_token, c)
      end

      prev_token = token
      l = r + 1
    end
  end
  copy
end

