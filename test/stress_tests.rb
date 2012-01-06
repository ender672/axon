require 'helper'

module Axon
  class TestStressCases < AxonTestCase
    def setup
      @velvet = "\x0A\x14\x69"
      @image = Solid.new 1000, 1500, @velvet

      io = StringIO.new
      JPEG.write(@image, io)
      @jpeg_data = io.string
      @readerclass = JPEG::Reader
    end

    class RandomRaiseIO
      def initialize(source)
        @io = StringIO.new source
        @die_at = rand(source.size)
      end

      def read(*args)
        if @die_at < 2
          raise 'random death!'
        end

        read_size = rand @die_at
        @die_at -= read_size
        @io.read(read_size)
      end
    end

    def test_io_raises_after_random_sized_reads
      loop do
        f = RandomRaiseIO.new(@jpeg_data)
        r = @readerclass.new(f) rescue puts("new failed")
        r.each{} rescue puts('each_sl failed')
      end
    end

    class CustomIO < StringIO
      def initialize(str, sequence)
        super str
        @sequence = sequence
      end

      def read(*args)
        if !@sequence || @sequence.empty?
          raise "random death!"
          @sequence = nil
        end
        super(@sequence.shift)
      end
    end

    def test_custom_io_error
      loop do
        f = CustomIO.new(@jpeg_data, [114, 64, 8, 200, 200, 30, 350])

        r = @readerclass.new(f)
        r.each{  } rescue nil
        f = CustomIO.new(@jpeg_data, [101, 0])
        r = @readerclass.new(f) rescue nil
      end
    end

    def test_io_returns_nil_after_random_sized_reads
      loop do
        f = SometimesNilIO.new(@data)
        r = @readerclass.new(f)
        r.each{}
      end
    end
  end
end
