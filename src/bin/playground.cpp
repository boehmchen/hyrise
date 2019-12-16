#include <iostream>
#include <logical_query_plan/mock_node.hpp>
#include <synthetic_table_generator.hpp>
#include <expression/expression_functional.hpp>
#include <logical_query_plan/lqp_translator.hpp>
#include <fstream>

#include "hyrise.hpp"
#include "scheduler/operator_task.hpp"

#include "logical_query_plan/stored_table_node.hpp"
#include "logical_query_plan/static_table_node.hpp"

#include "logical_query_plan/projection_node.hpp"
#include "logical_query_plan/predicate_node.hpp"

#include "expression/expression_functional.hpp"

#include "types.hpp"
#include "magic_enum.hpp"

using namespace opossum;  // NOLINT
using namespace opossum::expression_functional;


// time control parameters
int selectivityGranularity = 10;
int column_size = 10000;
int valueBytes = 10;

// data specific parameters
enum DataTypes {stringType, integerType, floatType, doubleType};

enum EncodingTypes {valueEncoding, dictionaryEncoding, referenceEncoding, indexEncoding};  // TODO add all, check whether index is a type of its own
enum CompressionTypes {};  // TODO add possible types
enum DataDistributionTypes {uniformDistribution, randomDistribution};
enum Sortings {noSorting, ascSorting};

// test specific parameters
enum Operators {TableScanOperator};  //, Join, Aggregation};

// misc parameters
enum TableScanPredicates {ltPredicate, BETWEENPredicate, LIKEPredicate, ISNULLPredicate};

//// Data Generation
//for DistributionTypes:
//  for DataTypes
//    // all data_specific_paramters
//    [...]
//    generateColumn(createName(datatype, distributionType ...));
//  ;
//
//// LQP Generation
//for column in table:
//    for paramters in execution_specific_parameters:
//      result_timing = generateLQP(column, ...paramters).execute()
//      write_to_csv(column, parameters, timing) // knows whether combination dosnt make sense
//
//
//
//generateColumn(){
//
//    }
//
//
//write_to_csv () {
//
//    }
//
//generateLQP () {
//
//    }

int main() {
//    enum Color { RED, BLUE, GREEN };
//
//    Color color = Color::RED;
//    auto color_name = magic_enum::enum_name(color);
//    std::cout << color_name << std::endl;

    constexpr auto row_count = size_t{1000000};
    constexpr auto chunk_size = size_t{1000};

    constexpr auto DELIMITER = ";";
    constexpr auto NEW_LINE = "\n";

    // Table Generation
    // Step 1 generate combinations to test for
    std::vector<DataDistributionTypes> distribution = std::vector<DataDistributionTypes>();
    std::vector<DataTypes> data_types = std::vector<DataTypes>();
    std::vector<EncodingTypes > encoding = std::vector<EncodingTypes>();
    std::vector<std::string> column_names = std::vector<std::string>();
    int trail_count = 0;

    for (DataDistributionTypes dist : magic_enum::enum_values<DataDistributionTypes>()) {
      for (DataTypes dt : magic_enum::enum_values<DataTypes>()) {
        for (EncodingTypes enc : magic_enum::enum_values<EncodingTypes>()) {
          distribution.push_back(dist);
          data_types.push_back(dt);
          encoding.push_back(enc);

          // generate Name
          std::stringstream ss;
          ss << magic_enum::enum_name(dist) << "_" << magic_enum::enum_name(dt) << "_" << magic_enum::enum_name(enc);
          column_names.push_back(ss.str());

          trail_count++;
        }
      }
    }

    // Step 2 convert to correct values for generation
    // TODO


    // Step 3 generate

    auto table_generator = std::make_shared<SyntheticTableGenerator>();
    auto uniform_distribution_0_1 = ColumnDataDistribution::make_uniform_config(0.0, 10000.0);

    auto table = table_generator->generate_table(
            {uniform_distribution_0_1, uniform_distribution_0_1, uniform_distribution_0_1},
            {DataType::Double, DataType::Double, DataType::String},
            row_count,
            chunk_size,
            {{EncodingType::Dictionary, EncodingType::Dictionary, EncodingType::Dictionary}},
            {{"_a", "_b", "_c"}},
            UseMvcc::Yes    // MVCC = Multiversion concurrency control
                                      // this must be true because only MVCC tables can be added to storage manager
    );


    Hyrise::get().storage_manager.add_table("t_a", table);
    //const auto _t_a_a = Hyrise::get().storage_manager.get_table("t_a");

    const auto _t_a = StoredTableNode::make("t_a");
    //const auto _t_a_a = StaticTableNode::make(table);

    const auto _a = _t_a->get_column("_a");
    const auto _b = _t_a->get_column("_b");

    std::ofstream output_performance_data_file;
    output_performance_data_file.open ("/Users/lukas/Documents/git/hyrise/measurements/output_performance_data.csv");

    for (int i = 0; i <= int(row_count); i+=  row_count / 1000 ){
        const auto _projection_node_a =
                ProjectionNode::make(expression_vector(_a),
                                     PredicateNode::make(greater_than_(_b, i), _t_a));

        const auto pqp = LQPTranslator{}.translate_node(_projection_node_a);
        const auto tasks = OperatorTask::make_tasks_from_operator(pqp, CleanupTemporaries::Yes);
        Hyrise::get().scheduler()->schedule_and_wait_for_tasks(tasks);

        auto next = pqp->input_left();
        while (next != nullptr) {

            output_performance_data_file << next->name() << DELIMITER;
            output_performance_data_file << i << DELIMITER;
            output_performance_data_file << next->performance_data().walltime.count() << NEW_LINE;

            next = next->input_left();
        }
    }
    //pqp->input_left()->performance_data().output_to_stream(output_performance_data_file);
    output_performance_data_file.close();
}


