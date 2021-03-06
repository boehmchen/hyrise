#pragma once

#include "string"

#include <expression/expression_functional.hpp>
#include <logical_query_plan/abstract_lqp_node.hpp>
#include <operators/abstract_operator.hpp>

#include "calibration_table_wrapper.hpp"
#include "logical_query_plan/stored_table_node.hpp"

namespace opossum {
class CalibrationLQPGenerator {
 public:
  CalibrationLQPGenerator();
  void generate(OperatorType operator_type, const std::shared_ptr<const CalibrationTableWrapper>& table);
  const std::vector<std::shared_ptr<AbstractLQPNode>>& get_lqps();

 private:
  using ColumnPair = std::pair<const std::string, const std::string>;
  void _generate_table_scans(const std::shared_ptr<const CalibrationTableWrapper>& table_wrapper);
  void _generate_column_vs_column_scans(const std::shared_ptr<const CalibrationTableWrapper>& table_wrapper);
  [[nodiscard]] std::vector<CalibrationLQPGenerator::ColumnPair> _get_column_pairs(
      const std::shared_ptr<const CalibrationTableWrapper>& table_wrapper) const;

  std::vector<std::shared_ptr<AbstractLQPNode>> _generated_lpqs;

  // feature flags for the LQPGeneration
  static constexpr bool _enable_like_predicates = true;
  static constexpr bool _enable_reference_scans = true;
  static constexpr bool _enable_column_vs_column_scans = true;
  static constexpr bool _enable_between_predicates = true;
};
}  // namespace opossum
