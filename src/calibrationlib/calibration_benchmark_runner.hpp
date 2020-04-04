#pragma once

#include "string"

#include <benchmark_config.hpp>
#include <benchmark_runner.hpp>
#include "operator_feature_exporter.hpp"
#include "table_feature_exporter.hpp"

namespace opossum {
enum BenchmarkType { TCPH, TCPDS, JOB };

class CalibrationBenchmarkRunner {
 public:
  explicit CalibrationBenchmarkRunner(const std::string& path_to_dir);
  CalibrationBenchmarkRunner(const std::string& path_to_dir, std::shared_ptr<BenchmarkConfig> config);

  void run_benchmark(const BenchmarkType type, const float SCALE_FACTOR, const int number_of_executions);

 private:
  const OperatorFeatureExporter _feature_exporter;
  TableFeatureExporter _table_exporter;

  std::shared_ptr<BenchmarkConfig> _config;

  std::shared_ptr<BenchmarkRunner> _build_tcph(const float SCALE_FACTOR) const;
  std::shared_ptr<BenchmarkRunner> _build_tcpds(const float SCALE_FACTOR) const;
  std::shared_ptr<BenchmarkRunner> _build_job() const;
};
}  // namespace opossum
