module CompareRubies
  def run_nat(path, *args)
    out_nat = `bin/natalie -I test/support #{path} #{args.join(' ')} 2>&1`
    puts out_nat unless $?.success?
    expect($?).must_be :success?
    out_nat
  end

  def run_self_hosted_nat(path, *args)
    unless File.exist?(File.expand_path('../tmp/nat', __dir__))
      out_nat = `bin/natalie -c test/tmp/nat bin/natalie 2>&1`
      puts out_nat unless $?.success?
    end
    unless File.exist?(File.expand_path('../tmp/nat2', __dir__))
      out_nat = `./test/tmp/nat -c test/tmp/nat2 bin/natalie 2>&1`
      puts out_nat unless $?.success?
    end
    out_nat = `./test/tmp/nat2 #{path} #{args.join(' ')} 2>&1`
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
    out_self_hosted_nat = run_self_hosted_nat(path, *args)
    out_ruby = run_ruby(path, *args)
    expect(out_nat).must_equal(out_self_hosted_nat)
    expect(out_nat).must_equal(out_ruby)
  end
end
