
class MiniTest::Unit::TestCase
  WARMUP_TESTS = 10
  MAX_TEST_TIME = 3
  MAX_TESTS = 3000

  def vmsize
    GC.start
    GC.disable
    vms = IO.read("/proc/#{$$}/status")[/^VmSize:\s+(\d+)/, 1].to_i
    GC.enable
    vms
  end

  def vmdelta
    start_vmsize = vmsize
    yield
    vmsize - start_vmsize
  end

  def limited_loop
    start_time = Time.now
    test_counter = 0

    until Time.now - start_time > MAX_TEST_TIME || test_counter > MAX_TESTS do
      yield
      test_counter += 1
    end

    return test_counter
  end

  def mem_loop
    result = yield
    WARMUP_TESTS.times{ yield }
    num_tests = 0

    delta = vmdelta do
      num_tests = limited_loop do
        yield
      end
    end

    # The idea here is that a legitimate memory leak will leak at least one
    # byte per iteration.
    if delta >= num_tests
      puts "\n#{self.class} #{__name__}: #{delta}"
    end

    result
  end

  alias_method :old_run, :run

  def run(*args)
    mem_loop{ old_run(*args) }
  end
end
