module CompareRubies
  def run_nat(path, *args)
    out_nat = `bin/natalie -I test/support #{path} #{args.join(' ')} 2>&1`
    puts out_nat unless $?.success?
    expect($?).must_be :success?
    out_nat
  end

  def run_self_hosted_nat(path, *args)
    out_nat = `bin/natalie bin/natalie #{path} #{args.join(' ')} 2>&1`
    puts out_nat unless $?.success?
    expect($?).must_be :success?
    out_nat
  end

  def run_ruby(path, *args)
    out_ruby = `ruby -r ruby_parser -I test/support #{path} #{args.join(' ')} 2>&1`
    puts out_ruby unless $?.to_i == 0
    expect($?).must_be :success?
    out_ruby
  end

  def run_both_and_compare(path, *args)
    out_nat = run_nat(path, *args)
    out_ruby = run_ruby(path, *args)
    expect(out_nat).must_equal(out_ruby)
  end

  def run_all_and_compare(path, *args)
    out_nat = run_nat(path, *args)
    out_ruby = run_ruby(path, *args)
    expect(out_nat).must_equal(out_ruby)
    if RUBY_PLATFORM !~ /openbsd/
      # FIXME error on boardslam.rb: mmap(PROT_NONE) failed
      out_self_hosted_nat = run_self_hosted_nat(path, *args)
      expect(out_nat).must_equal(out_self_hosted_nat)
    end
  end
end
