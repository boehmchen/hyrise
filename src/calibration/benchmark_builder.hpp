#pragma once

#include <benchmark_config.hpp>
#include "string"
#include "table_export.hpp"
#include "measurement_export.hpp"

namespace opossum{
enum BenchmarkType{
    TCPH, TCPDS, JOB
};

class BenchmarkBuilder {
  /**
   * BenchmarkBuilder provides a function to export all resulting PQP in the same format as
   * the measurement export for cost model calibration calibration.
   *
   * The export of this file is mainly used as test data for testing model prediction
   */

  public:
    /**
     * Constructor with default benchmark configuration
     *
     * @param path_to_dir BenchmarkBuilder exports csv files to this directory
     */
    BenchmarkBuilder(const std::string& path_to_dir);

    /**
     * Constructor with custom benchmark configuration
     *
     * @param path_to_dir BenchmarkBuilder exports csv files to this directory
     * @param config BenchmarkConfig used for execution of benchmarks
     */
    BenchmarkBuilder(const std::string& path_to_dir, std::shared_ptr<BenchmarkConfig> config);

    /**
     * Executes and exports a benchmark of given benchmark type.
     * The export consists of multiple files:
     * - table_meta.csv
     * - column_meta.csv
     * - segment_meta.csv
     * - One CSV per OperatorType
     *
     * @param type of benchmark
     * @param SCALE_FACTOR scale_factor of benchmark (only needed for type TCP-H and TCP-DS)
     */
    void export_benchmark(const BenchmarkType type, const float SCALE_FACTOR) const;

  private:
    const std::string&      _path_to_dir;
    const MeasurementExport _measurement_export;
    const TableExport       _table_export;

    std::shared_ptr<BenchmarkConfig> _config;

    void _run_tcph(const float SCALE_FACTOR) const;
    void _run_tcpds(const float SCALE_FACTOR) const;
    void _run_job(const float SCALE_FACTOR) const;
};
}  // namespace opossum
