#!/usr/bin/env ruby

arr = [1, 2, 3]
enum = arr.enum_for(:each) { arr.size }
p enum.size # => 3
p enum.map(&:itself).to_s # => "[1, 2, 3]"
