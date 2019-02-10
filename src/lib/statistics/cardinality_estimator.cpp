#include "cardinality_estimator.hpp"

#include <iostream>
#include <memory>

#include "histograms/equal_distinct_count_histogram.hpp"
#include "histograms/generic_histogram.hpp"
#include "histograms/single_bin_histogram.hpp"
#include "chunk_statistics2.hpp"
#include "expression/abstract_expression.hpp"
#include "expression/expression_utils.hpp"
#include "expression/lqp_subquery_expression.hpp"
#include "expression/value_expression.hpp"
#include "logical_query_plan/abstract_lqp_node.hpp"
#include "logical_query_plan/aggregate_node.hpp"
#include "logical_query_plan/alias_node.hpp"
#include "logical_query_plan/join_node.hpp"
#include "logical_query_plan/limit_node.hpp"
#include "logical_query_plan/mock_node.hpp"
#include "logical_query_plan/predicate_node.hpp"
#include "logical_query_plan/projection_node.hpp"
#include "logical_query_plan/sort_node.hpp"
#include "logical_query_plan/stored_table_node.hpp"
#include "logical_query_plan/validate_node.hpp"
#include "operators/operator_join_predicate.hpp"
#include "operators/operator_scan_predicate.hpp"
#include "optimizer/optimization_context.hpp"
#include "resolve_type.hpp"
#include "segment_statistics2.hpp"
#include "statistics/cardinality_estimation/cardinality_estimation_join.hpp"
#include "statistics/cardinality_estimation/cardinality_estimation_scan.hpp"
#include "storage/storage_manager.hpp"
#include "storage/table.hpp"
#include "table_statistics2.hpp"
#include "utils/assert.hpp"

namespace {

using namespace opossum;  // NOLINT

std::shared_ptr<TableStatistics2> estimate_alias_node(const AliasNode& alias_node,
                                                      const std::shared_ptr<TableStatistics2>& input_table_statistics) {
  const auto output_table_statistics = std::make_shared<TableStatistics2>();

  for (const auto& input_chunk_statistics_set : input_table_statistics->chunk_statistics_sets) {
    auto output_chunk_statistics_set = ChunkStatistics2Set{};
    output_chunk_statistics_set.reserve(input_chunk_statistics_set.size());

    for (const auto& input_chunk_statistics : input_chunk_statistics_set) {
      const auto output_chunk_statistics = std::make_shared<ChunkStatistics2>(input_chunk_statistics->row_count);
      output_chunk_statistics->segment_statistics.reserve(alias_node.column_expressions().size());

      for (const auto& expression : alias_node.column_expressions()) {
        const auto input_column_id = alias_node.left_input()->get_column_id(*expression);
        output_chunk_statistics->segment_statistics.emplace_back(
            input_chunk_statistics->segment_statistics[input_column_id]);
      }

      output_chunk_statistics_set.emplace_back(output_chunk_statistics);
    }

    output_table_statistics->chunk_statistics_sets.emplace_back(std::move(output_chunk_statistics_set));
  }

  return output_table_statistics;
}

std::shared_ptr<TableStatistics2> estimate_projection_node(
    const ProjectionNode& projection_node, const std::shared_ptr<TableStatistics2>& input_table_statistics) {
  const auto output_table_statistics = std::make_shared<TableStatistics2>();

  for (const auto& input_chunk_statistics_set : input_table_statistics->chunk_statistics_sets) {
    auto output_chunk_statistics_set = ChunkStatistics2Set{};
    output_chunk_statistics_set.reserve(input_chunk_statistics_set.size());

    for (const auto& input_chunk_statistics : input_chunk_statistics_set) {
      const auto output_chunk_statistics = std::make_shared<ChunkStatistics2>(input_chunk_statistics->row_count);
      output_chunk_statistics->segment_statistics.reserve(projection_node.column_expressions().size());

      for (const auto& expression : projection_node.column_expressions()) {
        const auto input_column_id = projection_node.left_input()->find_column_id(*expression);
        if (input_column_id) {
          output_chunk_statistics->segment_statistics.emplace_back(
              input_chunk_statistics->segment_statistics[*input_column_id]);
        } else {
          resolve_data_type(expression->data_type(), [&](const auto data_type_t) {
            using ColumnDataType = typename decltype(data_type_t)::type;
            output_chunk_statistics->segment_statistics.emplace_back(
                std::make_shared<SegmentStatistics2<ColumnDataType>>());
          });
        }
      }

      output_chunk_statistics_set.emplace_back(output_chunk_statistics);
    }

    output_table_statistics->chunk_statistics_sets.emplace_back(std::move(output_chunk_statistics_set));
  }

  return output_table_statistics;
}

std::shared_ptr<TableStatistics2> estimate_aggregate_node(
    const AggregateNode& aggregate_node, const std::shared_ptr<TableStatistics2>& input_table_statistics) {
  const auto output_table_statistics = std::make_shared<TableStatistics2>();

  for (const auto& input_chunk_statistics_set : input_table_statistics->chunk_statistics_sets) {
    auto output_chunk_statistics_set = ChunkStatistics2Set{};
    output_chunk_statistics_set.reserve(input_chunk_statistics_set.size());

    for (const auto& input_chunk_statistics : input_chunk_statistics_set) {
      const auto output_chunk_statistics = std::make_shared<ChunkStatistics2>(input_chunk_statistics->row_count);
      output_chunk_statistics->segment_statistics.reserve(aggregate_node.column_expressions().size());

      for (const auto& expression : aggregate_node.column_expressions()) {
        const auto input_column_id = aggregate_node.left_input()->find_column_id(*expression);
        if (input_column_id) {
          output_chunk_statistics->segment_statistics.emplace_back(
              input_chunk_statistics->segment_statistics[*input_column_id]);
        } else {
          resolve_data_type(expression->data_type(), [&](const auto data_type_t) {
            using ColumnDataType = typename decltype(data_type_t)::type;
            output_chunk_statistics->segment_statistics.emplace_back(
                std::make_shared<SegmentStatistics2<ColumnDataType>>());
          });
        }
      }

      output_chunk_statistics_set.emplace_back(output_chunk_statistics);
    }

    output_table_statistics->chunk_statistics_sets.emplace_back(std::move(output_chunk_statistics_set));
  }

  return output_table_statistics;
}

std::shared_ptr<TableStatistics2> estimate_validate_node(
    const ValidateNode& validate_node, const std::shared_ptr<TableStatistics2>& input_table_statistics) {
  const auto output_table_statistics = std::make_shared<TableStatistics2>();

  for (const auto& input_chunk_statistics_set : input_table_statistics->chunk_statistics_sets) {
    auto output_chunk_statistics_set = ChunkStatistics2Set{};
    output_chunk_statistics_set.reserve(input_chunk_statistics_set.size());

    for (const auto& input_chunk_statistics : input_chunk_statistics_set) {
      if (input_chunk_statistics->row_count == 0) {
        // No need to write out statistics for Chunks without rows
        continue;
      }

      const auto output_chunk_statistics = std::make_shared<ChunkStatistics2>(
          input_chunk_statistics->row_count - input_chunk_statistics->approx_invalid_row_count);
      output_chunk_statistics->segment_statistics.reserve(input_chunk_statistics->segment_statistics.size());

      const auto selectivity = (input_chunk_statistics->row_count - input_chunk_statistics->approx_invalid_row_count) /
                               input_chunk_statistics->row_count;

      for (const auto& segment_statistics : input_chunk_statistics->segment_statistics) {
        output_chunk_statistics->segment_statistics.emplace_back(
            segment_statistics->scaled_with_selectivity(selectivity));
      }

      output_chunk_statistics_set.emplace_back(output_chunk_statistics);
    }

    output_table_statistics->chunk_statistics_sets.emplace_back(std::move(output_chunk_statistics_set));
  }

  return output_table_statistics;
}

std::shared_ptr<TableStatistics2> estimate_predicate_node(
    const PredicateNode& predicate_node, const std::shared_ptr<TableStatistics2>& input_table_statistics) {
  const auto operator_scan_predicates =
      OperatorScanPredicate::from_expression(*predicate_node.predicate(), predicate_node);

  // TODO(anybody) Complex predicates are not processed right now and statistics objects are forwarded
  //               That implies estimating a selectivity of 1 for such predicates
  if (!operator_scan_predicates) {
    return input_table_statistics;
  } else {
    const auto output_table_statistics = std::make_shared<TableStatistics2>();

    for (const auto& input_chunk_statistics_set : input_table_statistics->chunk_statistics_sets) {
      auto output_chunk_statistics_set = ChunkStatistics2Set{};
      output_chunk_statistics_set.reserve(input_chunk_statistics_set.size());

      for (const auto& input_chunk_statistics : input_chunk_statistics_set) {
        auto output_chunk_statistics = input_chunk_statistics;
        for (const auto& operator_scan_predicate : *operator_scan_predicates) {
          output_chunk_statistics = cardinality_estimation_chunk_scan(output_chunk_statistics, operator_scan_predicate);
          if (!output_chunk_statistics) break;
        }

        if (output_chunk_statistics) {
          output_chunk_statistics_set.emplace_back(output_chunk_statistics);
        } else {
          // If there are no matches expected in this Chunk, don't return a ChunkStatistics object for this
          // Chunk
        }
      }

      output_table_statistics->chunk_statistics_sets.emplace_back(output_chunk_statistics_set);
    }

    return output_table_statistics;
  }
}

std::shared_ptr<TableStatistics2> estimate_join_node(
    const JoinNode& join_node, const std::shared_ptr<TableStatistics2>& left_input_table_statistics,
    const std::shared_ptr<TableStatistics2>& right_input_table_statistics) {
  if (join_node.join_mode == JoinMode::Cross) {
    return cardinality_estimation_cross_join(left_input_table_statistics, right_input_table_statistics);
  } else {
    const auto operator_join_predicate = OperatorJoinPredicate::from_join_node(join_node);

    if (operator_join_predicate) {
      return cardinality_estimation_predicated_join(join_node.join_mode, *operator_join_predicate,
                                                    left_input_table_statistics, right_input_table_statistics);
    } else {
      // TODO(anybody) For now, estimate a selectivity of one.
      return cardinality_estimation_cross_join(left_input_table_statistics, right_input_table_statistics);
    }
  }
}

std::shared_ptr<TableStatistics2> estimate_limit_node(const LimitNode& limit_node,
                                                      const std::shared_ptr<TableStatistics2>& input_table_statistics) {
  const auto output_table_statistics = std::make_shared<TableStatistics2>();

  auto num_rows = Cardinality{};
  if (const auto value_expression = std::dynamic_pointer_cast<ValueExpression>(limit_node.num_rows_expression())) {
    num_rows = type_cast_variant<Cardinality>(value_expression->value);
  } else {
    num_rows = input_table_statistics->row_count();
  }

  const auto chunk_statistics = std::make_shared<ChunkStatistics2>(num_rows);
  chunk_statistics->segment_statistics.resize(input_table_statistics->column_count());

  for (auto column_id = ColumnID{0}; column_id < input_table_statistics->column_count(); ++column_id) {
    resolve_data_type(input_table_statistics->column_data_type(column_id), [&](const auto data_type_t) {
      using ColumnDataType = typename decltype(data_type_t)::type;
      chunk_statistics->segment_statistics[column_id] = std::make_shared<SegmentStatistics2<ColumnDataType>>();
    });
  }

  auto chunk_statistics_set = ChunkStatistics2Set{};
  chunk_statistics_set.emplace_back(chunk_statistics);

  output_table_statistics->chunk_statistics_sets.emplace_back(chunk_statistics_set);

  return output_table_statistics;
}

}  // namespace

namespace opossum {

Cardinality CardinalityEstimator::estimate_cardinality(const std::shared_ptr<AbstractLQPNode>& lqp,
                                                       const std::shared_ptr<OptimizationContext>& context) const {
  // //std::cout << "CardinalityEstimator::estimate_cardinality() {" << std::endl;
  // lqp->print();
  // //std::cout << "}" << std::endl;
  return estimate_statistics(lqp, context)->row_count();
}

std::shared_ptr<TableStatistics2> CardinalityEstimator::estimate_statistics(
    const std::shared_ptr<AbstractLQPNode>& lqp, const std::shared_ptr<OptimizationContext>& context) const {

  auto output_table_statistics = std::shared_ptr<TableStatistics2>{};
  const auto input_table_statistics = lqp->left_input() ? estimate_statistics(lqp->left_input(), context) : nullptr;

  switch (lqp->type) {
    case LQPNodeType::Aggregate: {
      const auto aggregate_node = std::dynamic_pointer_cast<AggregateNode>(lqp);
      output_table_statistics = estimate_aggregate_node(*aggregate_node, input_table_statistics);
    } break;

    case LQPNodeType::Alias: {
      const auto alias_node = std::dynamic_pointer_cast<AliasNode>(lqp);
      output_table_statistics = estimate_alias_node(*alias_node, input_table_statistics);
    } break;

    case LQPNodeType::Join: {
      const auto join_node = std::dynamic_pointer_cast<JoinNode>(lqp);
      const auto right_input_table_statistics = estimate_statistics(lqp->right_input(), context);
      output_table_statistics = estimate_join_node(*join_node, input_table_statistics, right_input_table_statistics);
    } break;

    case LQPNodeType::Limit: {
      const auto limit_node = std::dynamic_pointer_cast<LimitNode>(lqp);
      output_table_statistics = estimate_limit_node(*limit_node, input_table_statistics);
    } break;

    case LQPNodeType::Mock: {
      const auto mock_node = std::dynamic_pointer_cast<MockNode>(lqp);
      output_table_statistics = mock_node->table_statistics2();
    } break;

    case LQPNodeType::Predicate: {
      const auto predicate_node = std::dynamic_pointer_cast<PredicateNode>(lqp);
      output_table_statistics = estimate_predicate_node(*predicate_node, input_table_statistics);
    } break;

    case LQPNodeType::Projection: {
      const auto projection_node = std::dynamic_pointer_cast<ProjectionNode>(lqp);
      output_table_statistics = estimate_projection_node(*projection_node, input_table_statistics);
    } break;

    case LQPNodeType::Sort: {
      output_table_statistics = input_table_statistics;
    } break;

    case LQPNodeType::StoredTable: {
      const auto stored_table_node = std::dynamic_pointer_cast<StoredTableNode>(lqp);
      const auto stored_table = StorageManager::get().get_table(stored_table_node->table_name);
      Assert(stored_table->table_statistics2(), "");
      output_table_statistics = stored_table->table_statistics2();
    } break;

    case LQPNodeType::Validate: {
      const auto validate_node = std::dynamic_pointer_cast<ValidateNode>(lqp);
      output_table_statistics = estimate_validate_node(*validate_node, input_table_statistics);
    } break;

    case LQPNodeType::Union:
    case LQPNodeType::Update:
    case LQPNodeType::Insert:
    case LQPNodeType::Root:
    case LQPNodeType::ShowColumns:
    case LQPNodeType::ShowTables:
    case LQPNodeType::CreateTable:
    case LQPNodeType::CreatePreparedPlan:
    case LQPNodeType::CreateView:
    case LQPNodeType::Delete:
    case LQPNodeType::DropView:
    case LQPNodeType::DropTable:
    case LQPNodeType::DummyTable:
      // TODO(anybody)
      break;
  }

  Assert(output_table_statistics, "NYI");

  if (context) {
    if (context->plan_statistics_cache) {
      context->plan_statistics_cache->emplace(lqp, output_table_statistics);
    }
  }

  //std::cout << "}" << std::endl;

  return output_table_statistics;
}

}  // namespace opossum
