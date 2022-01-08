require 'timeout'

module CompareRubies
  SPEC_TIMEOUT = (ENV['SPEC_TIMEOUT'] || 120).to_i

  def run_nat(path, *args)
    out_nat = sh("bin/natalie -I test/support #{path} #{args.join(' ')} 2>&1")
    puts out_nat unless $?.success?
    expect($?).must_be :success?
    out_nat
  end

  def run_ruby(path, *args)
    out_ruby = sh("ruby -r ruby_parser -I test/support #{path} #{args.join(' ')} 2>&1")
    puts out_ruby unless $?.to_i == 0
    expect($?).must_be :success?
    out_ruby
  end

  def run_both_and_compare(path, *args)
    out_nat = run_nat(path, *args)
    out_ruby = run_ruby(path, *args)
    expect(out_nat).must_equal(out_ruby)
  end

  def sh(command)
    Timeout.timeout(SPEC_TIMEOUT, nil, "execution expired running: #{command}") { `#{command}` }
  end
end
