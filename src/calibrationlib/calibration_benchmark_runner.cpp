#include "calibration_benchmark_runner.hpp"
#include <benchmark_runner.hpp>
#include <file_based_benchmark_item_runner.hpp>
#include <file_based_table_generator.hpp>
#include <tpcds/tpcds_table_generator.hpp>
#include <tpch/tpch_benchmark_item_runner.hpp>
#include <tpch/tpch_table_generator.hpp>
#include "hyrise.hpp"

namespace opossum {

CalibrationBenchmarkRunner::CalibrationBenchmarkRunner(const std::string& path_to_dir)
    : _feature_exporter(OperatorFeatureExporter(path_to_dir)), _table_exporter(TableFeatureExporter(path_to_dir)) {
  _config = std::make_shared<BenchmarkConfig>(BenchmarkConfig::get_default_config());
}

CalibrationBenchmarkRunner::CalibrationBenchmarkRunner(const std::string& path_to_dir,
                                                       std::shared_ptr<BenchmarkConfig> config)
    : _feature_exporter(OperatorFeatureExporter(path_to_dir)), _table_exporter(TableFeatureExporter(path_to_dir)) {
  _config = config;
}

void CalibrationBenchmarkRunner::run_benchmark(const BenchmarkType type, const float scale_factor,
                                               const int number_of_executions) {
  std::shared_ptr<BenchmarkRunner> benchmark_runner;
  switch (type) {
    case BenchmarkType::TCPH:
      benchmark_runner = _build_tcph(scale_factor);
      break;
    case BenchmarkType::TCPDS:
      benchmark_runner = _build_tcpds(scale_factor);
      break;
    case BenchmarkType::JOB:
      benchmark_runner = _build_job();
      break;
    default:
      throw std::runtime_error("Provided unknown BenchmarkType.");
  }

  for (int execution_index = 0; execution_index < number_of_executions; ++execution_index) {
    Hyrise::get().benchmark_runner = benchmark_runner;
    benchmark_runner->run();

    auto& pqp_cache = Hyrise::get().default_pqp_cache;

    for (auto pqp_entry = pqp_cache->unsafe_begin(); pqp_entry != pqp_cache->unsafe_end(); ++pqp_entry) {
      const auto& [query_string, physical_query_plan] = *pqp_entry;
      _feature_exporter.export_to_csv(physical_query_plan);
    }

    // Clear pqp cache for next benchmark run
    pqp_cache->clear();
  }

  const std::vector<std::string> table_names = Hyrise::get().storage_manager.table_names();
  for (const auto& table_name : table_names) {
    auto table = Hyrise::get().storage_manager.get_table(table_name);
    _table_exporter.export_table(std::make_shared<CalibrationTableWrapper>(CalibrationTableWrapper(table, table_name)));

    Hyrise::get().storage_manager.drop_table(table_name);
  }
}

std::shared_ptr<BenchmarkRunner> CalibrationBenchmarkRunner::_build_tcph(const float scale_factor) const {
  auto item_runner = std::make_unique<TPCHBenchmarkItemRunner>(_config, false, scale_factor);
  auto benchmark_runner = std::make_shared<BenchmarkRunner>(*_config, std::move(item_runner),
                                                            std::make_unique<TPCHTableGenerator>(scale_factor, _config),
                                                            BenchmarkRunner::create_context(*_config));

  return benchmark_runner;
}

std::shared_ptr<BenchmarkRunner> CalibrationBenchmarkRunner::_build_tcpds(const float scale_factor) const {
  const std::string query_path = "hyrise/resources/benchmark/tpcds/tpcds-result-reproduction/query_qualification";

  auto query_generator =
      std::make_unique<FileBasedBenchmarkItemRunner>(_config, query_path, std::unordered_set<std::string>{});
  auto table_generator = std::make_unique<TpcdsTableGenerator>(scale_factor, _config);
  auto benchmark_runner =
      std::make_shared<BenchmarkRunner>(*_config, std::move(query_generator), std::move(table_generator),
                                        opossum::BenchmarkRunner::create_context(*_config));
  return benchmark_runner;
}

std::shared_ptr<BenchmarkRunner> CalibrationBenchmarkRunner::_build_job() const {
  const auto table_path = "hyrise/imdb_data";
  const auto query_path = "hyrise/third_party/join-order-benchmark";
  const auto non_query_file_names = std::unordered_set<std::string>{"fkindexes.sql", "schema.sql"};

  auto benchmark_item_runner =
      std::make_unique<FileBasedBenchmarkItemRunner>(_config, query_path, non_query_file_names);
  auto table_generator = std::make_unique<FileBasedTableGenerator>(_config, table_path);
  auto benchmark_runner =
      std::make_shared<BenchmarkRunner>(*_config, std::move(benchmark_item_runner), std::move(table_generator),
                                        BenchmarkRunner::create_context(*_config));
  return benchmark_runner;
}
}  // namespace opossum
