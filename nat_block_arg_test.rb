require 'minitest/spec'
require 'minitest/autorun'

describe 'argument spreading' do
  # sig:  array of pairs; each pair is either an argument name and default value,
  #       or just a single argument name, e.g. ["a", 1] or just ["a"]
  # vals: array of argument values
  #
  # return: hash of names and values, e.g. {"a"=>[1, 2], "b"=>3, "c"=>4, "d"=>5}
  def apply_args(sig, vals)
    first_required = sig.index { |a| a.size < 2 }
    req_arg_count = sig.count { |a| a.size < 2 }
    arg_index = first_required
    val_index = [
      arg_index,
      [0, vals.size - req_arg_count].max
    ].min
    args_available = sig.size
    vals_available = vals.size
    result = sig.each_with_object({}) do |pair, hash|
      name = pair.first
      if name.start_with?('*')
        hash[name[1..-1]] = pair.size > 1 ? pair[1] : []
      else
        hash[name] = pair.size > 1 ? pair[1] : nil
      end
    end
    loop do
      break if vals_available.zero? || args_available.zero?
      if arg_index >= sig.size
        arg_index = 0
      end
      if val_index >= vals.size
        val_index = 0
      end
      name = sig[arg_index].first
      args_available -= 1
      if name.start_with?('*')
        result[name[1..-1]] = []
        until vals_available <= args_available
          result[name[1..-1]] << vals[val_index]
          val_index += 1
          vals_available -= 1
        end
      else
        result[name] = vals[val_index]
        vals[val_index] = nil
        val_index += 1
        vals_available -= 1
      end
      arg_index += 1
    end
    result
  end

  tests = [
    [[['a', 5], ['b']],                       [1, 2],          {"a"=>1, "b"=>2}                     ],
    [[['a', 5], ['b'], ['c'], ['d']],         [1, 2],          {"a"=>5, "b"=>1, "c"=>2, "d"=>nil}   ],
    [[['a', 5], ['b', 4], ['c'], ['d']],      [1, 2],          {"a"=>5, "b"=>4, "c"=>1, "d"=>2}     ],
    [[['a', 5], ['b', 4], ['c'], ['d']],      [1, 2, 3],       {"a"=>1, "b"=>4, "c"=>2, "d"=>3}     ],
    [[['a', 5], ['b', 4], ['c', 3], ['d']],   [1, 2, 3],       {"a"=>1, "b"=>2, "c"=>3, "d"=>3}     ],
    [[['a', 5], ['b', 4], ['c', 3], ['d']],   [1, 2, nil],     {"a"=>1, "b"=>2, "c"=>3, "d"=>nil}   ],
    [[['a'], ['b'], ['c'], ['d', 0]],         [1, 2, 3],       {"a"=>1, "b"=>2, "c"=>3, "d"=>0}     ],
    [[['a'], ['b'], ['c'], ['d', 0]],         [1, 2],          {"a"=>1, "b"=>2, "c"=>nil, "d"=>0}   ],
    [[['a'], ['b'], ['c'], ['d', 0]],         [1, 2, 3, 4],    {"a"=>1, "b"=>2, "c"=>3, "d"=>4}     ],
    [[['a'], ['b'], ['c'], ['d', 0]],         [1, 2, 3, 4, 5], {"a"=>1, "b"=>2, "c"=>3, "d"=>4}     ],
    [[['a', 5], ['b', 4], ['c'], ['d']],      [1, 2, 3],       {"a"=>1, "b"=>4, "c"=>2, "d"=>3}     ],
    [[['a', 5], ['b', 4], ['c'], ['d']],      [1, 2],          {"a"=>5, "b"=>4, "c"=>1, "d"=>2}     ],
    [[['a', 5], ['b', 4], ['c', nil], ['d']], [1, 2],          {"a"=>1, "b"=>4, "c"=>nil, "d"=>2}   ],
    [[['*a'], ['b'], ['c'], ['d']],           [1, 2],          {"a"=>[], "b"=>1, "c"=>2, "d"=>nil}  ],
    [[['*a'], ['b'], ['c'], ['d']],           [1, 2, 3],       {"a"=>[], "b"=>1, "c"=>2, "d"=>3}    ],
    [[['*a'], ['b'], ['c'], ['d']],           [1, 2, 3, 4],    {"a"=>[1], "b"=>2, "c"=>3, "d"=>4}   ],
    [[['*a'], ['b'], ['c'], ['d']],           [1, 2, 3, 4, 5], {"a"=>[1, 2], "b"=>3, "c"=>4, "d"=>5}],
    [[['a'], ['*b'], ['c'], ['d']],           [1, 2],          {"a"=>1, "b"=>[], "c"=>2, "d"=>nil}  ],
    [[['a'], ['*b'], ['c'], ['d']],           [1, 2, 3],       {"a"=>1, "b"=>[], "c"=>2, "d"=>3}    ],
    [[['a'], ['*b'], ['c'], ['d']],           [1, 2, 3, 4],    {"a"=>1, "b"=>[2], "c"=>3, "d"=>4}   ],
    [[['a'], ['*b'], ['c'], ['d']],           [1, 2, 3, 4, 5], {"a"=>1, "b"=>[2, 3], "c"=>4, "d"=>5}],
    [[['a'], ['b'], ['c'], ['*d']],           [1, 2],          {"a"=>1, "b"=>2, "c"=>nil, "d"=>[]}  ],
    [[['a'], ['b'], ['c'], ['*d']],           [1, 2, 3],       {"a"=>1, "b"=>2, "c"=>3, "d"=>[]}    ],
    [[['a'], ['b'], ['c'], ['*d']],           [1, 2, 3, 4],    {"a"=>1, "b"=>2, "c"=>3, "d"=>[4]}   ],
    [[['a'], ['b'], ['c'], ['*d']],           [1, 2, 3, 4, 5], {"a"=>1, "b"=>2, "c"=>3, "d"=>[4, 5]}],
  ]

  tests.each do |signature, args, expected|
    it "applies #{args.inspect} to #{signature.inspect}" do
      ruby_code = "proc { |#{signature.map { |a| a.size > 1 ? "#{a.first}=#{a.last.inspect}" : a }.join(', ')}| [#{signature.map { |a| a.first.sub(/^\*/, '') }.join(', ')}] }.call(#{args.inspect})"
      ruby_result = eval(ruby_code)
      expect(ruby_result).must_equal expected.values
      expect(apply_args(signature, args)).must_equal expected
    end
  end
end
