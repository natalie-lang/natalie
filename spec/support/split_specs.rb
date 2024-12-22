# This is used by our GitHub build workflow to split spec files into groups.
#
# Usage:
#
#     ruby spec/support/split_specs.rb <num_groups> groupN
#
# Example:
#
#     # Split specs into 3 groups and print group 1
#     ruby spec/support/split_specs.rb 3 group1

groups, num = ARGV

raise 'expected "groupN"' unless num =~ /group(\d+)/
num = $1.to_i - 1

raise "expected 1-#{groups}" unless (0...groups.to_i).include?(num)

list = Dir['spec/**/*_spec.rb', 'test/**/*_test.rb']

rand = Random.new(42)
sorted_list = list.sort_by { |item| rand.rand }

slice_size = (sorted_list.size / groups.to_f).ceil
groups = []
sorted_list.each_slice(slice_size).each do |group|
  groups << group
end

puts groups[num].join(',')
