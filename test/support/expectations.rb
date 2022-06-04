require 'minitest/assertions'

module Minitest::Assertions
  def assert_equal_and_same_class(expected, actual)
    assert expected == actual && same_classes?(expected, actual),
      "Expected #{actual.inspect} to be the same Rational lit as #{expected.inspect}"
  end

  private

  def same_classes?(expected, actual)
    return false unless expected.class == actual.class
    if expected == Sexp
      expected.each_with_index.all? do |node, index|
        same_classes?(node, actual[index])
      end
    else
      true
    end
  end
end

require 'minitest/spec'

module Minitest::Expectations
  Enumerable.infect_an_assertion :assert_equal_and_same_class, :must_equal_and_be_same_class
end
