#pragma once

#include <operators/abstract_operator.hpp>
#include <string>

namespace opossum {
class MeasurementExport {
 /**
  * This class is used to export relevant information about an executed PQP. The export consists of multiple
  * CSV Files (one per operator). Columns within a CSV File depend on the specific operator
  */
 public:
  explicit MeasurementExport(const std::string& path_to_dir);

  /**
   * export_to_csv is a recursive function that exports the complete query operator tree by visiting every
   * abstract operator. The function calls _export_typed_operator for every operator.
   *
   * @param root operator of a query
   */
  void export_to_csv(std::shared_ptr<const AbstractOperator> op) const;

 private:
  const std::string& _path_to_dir;
  const std::string _separator = ",";

  void _export_typed_operator(std::shared_ptr<const AbstractOperator> op) const;

  [[nodiscard]] std::string _path(OperatorType operator_type) const ;

  void _export_generic(std::shared_ptr<const AbstractOperator> op) const;

  void _export_table_scan(std::shared_ptr<const AbstractOperator> op) const;

  void _append_to_file(std::string line, OperatorType operator_type) const;

  std::string _get_header(const OperatorType type) const;

  std::string _generic_header() const;
};
}  // namespace opossum
