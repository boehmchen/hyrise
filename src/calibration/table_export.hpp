#pragma once

#include "string"
#include "storage/table.hpp"
#include "calibration_table_wrapper.hpp"

namespace opossum {

enum TableExportType {
    TABLE, COLUMN, SEGMENT
};

class TableExport{
 /**
  * TableExport exports a table meta information by creating three CSV files in the provided directory.
  *
  * This class exports a Calibration Table
  * - table_meta.csv: contains table information e.g. chunk_size, row_size
  * - column_meta.csv: contains information about all columns of all tables (columns can be mapped to tables via python)
  * - segment_meta: contains information about segments (segments can be mapped to columns)
  *
  * @note TableExport does not delete old files at the moment
  */
 public:
    TableExport(const std::string& path_to_dir);

    /**
     * Exports given table as described above.
     *
     * @param table_wrapper CalibrationTableWrapper contains additional information for a table
     */
    void export_table(std::shared_ptr<const CalibrationTableWrapper> table_wrapper) const;

private:
  const std::string& _path_to_dir;

  const std::string _table_meta_file_name = "table_meta";
  const std::string _column_meta_file_name = "column_meta";
  const std::string _segment_meta_file_name = "segment_meta";

  const std::string _table_file_path = _path_to_dir + "/" + _table_meta_file_name + ".csv";
  const std::string _column_file_path = _path_to_dir + "/" + _column_meta_file_name + ".csv";
  const std::string _segment_file_path = _path_to_dir + "/" + _segment_meta_file_name + ".csv";

  const std::string _separator = ",";

  const std::string _get_header(const TableExportType type) const;

  const std::string& _get_relative_path(const TableExportType type) const ;

  void _create_file(const TableExportType type) const;

  const std::string _export_table_data(std::shared_ptr<const CalibrationTableWrapper> table_wrapper) const;
  const std::string _export_column_data(std::shared_ptr<const CalibrationTableWrapper> table_wrapper) const;
  const std::string _export_segment_data(std::shared_ptr<const CalibrationTableWrapper> table_wrapper) const;

  void _append_to_file(const TableExportType type, const std::string &str) const;
};
}