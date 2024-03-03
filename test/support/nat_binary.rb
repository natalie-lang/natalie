ENV['NAT_BINARY'] ||= 'bin/natalie'
ENV['NAT_BINARY'] = File.join(__dir__, '../..', ENV['NAT_BINARY']) unless File.absolute_path?(ENV['NAT_BINARY'])
NAT_BINARY = ENV['NAT_BINARY']
