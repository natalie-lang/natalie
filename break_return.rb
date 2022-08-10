def foo(type)
  loop do
    loop do
      case type
      when :break
        break 'inner_loop_break'
      when :return
        return 'inner_loop_return'
      else
        raise 'error'
      end
    end
    if type == :break
      break 'outer_loop_break'
    end
  end
rescue => e
  e.message
end

def bar(type)
  l = lambda do
    loop do
      loop do
        case type
        when :break
          break 'inner_loop_break'
        when :return
          return 'inner_loop_return'
        else
          raise 'error'
        end
      end
      if type == :break
        break 'outer_loop_break'
      end
    end
  end
  l.()
end

p foo(nil)
p foo(:break)
p foo(:return)
p bar(:break)
p bar(:return)
