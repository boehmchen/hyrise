#pragma once

#include "storage/table.hpp"

namespace opossum {


class CalibrationTableWrapper {
 /**
 * Wraps a table and holds information about value distribution and table name
 *
 * Why do we need this?
 *
 * For the LQP generation, we need additional information about the value distribution.
 * The TableGenerator generates values for columns with a given ColumnDataDistribution.
 * Sometimes we need the range of values (e.g. for TableScans) within a table to generated suitable LQPs
 * Furthermore, the table name is used to map table meta information (e.g. columns, row count, chunk size) to
 * a executed and measured PQP.
 **/
 public:
  CalibrationTableWrapper(
    const std::shared_ptr<Table> table,
    const std::string& table_name,

    /**
    * Constructor if we have the value distribution of a column. (This is usally the case if )
    *
    * @param table
    * @param table_name
    */
    const std::vector<ColumnDataDistribution> column_data_distribution_collection); //TODO change this to optional

    /**
     * Constructor if we do not have the value Distribution (e.g. if we use the wrapper for tables generated
     * by the benchmark table generator)
     *
     * @param table
     * @param table_name
     */
    CalibrationTableWrapper(
            const std::shared_ptr<Table> table,
            const std::string& table_name);

    // Getter for value distribution, for a column with id
    const ColumnDataDistribution get_column_data_distribution(ColumnID id) const;

    // Getter for table itself
    const std::shared_ptr<Table> get_table() const;

    // Getter for table name
    const std::string &get_name() const;

private:
  const std::shared_ptr<Table> _table;
  const std::string _name;
  const std::vector<ColumnDataDistribution> _column_data_distribution_collection;
};
}  // namespace opossum
