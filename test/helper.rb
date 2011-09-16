require 'minitest/autorun'
require 'axon'
require 'stringio'

module Axon
  class AxonTestCase < MiniTest::Unit::TestCase
    
    # Generate a solid velvet image
    def setup
      @velvet = "\x0A\x14\x69"
      @image = Solid.new 10, 15, @velvet
      @io_out = StringIO.new
    end
  end
end
