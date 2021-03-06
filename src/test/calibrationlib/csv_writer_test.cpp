#include <fstream>
#include <string>
#include <utility>

#include "base_test.hpp"
#include "csv_writer.hpp"

namespace opossum {

class CSVWriterTest : public BaseTest {
 protected:
  void SetUp() override {
    std::filesystem::remove_all(std::filesystem::temp_directory_path() / "hyriseCalibration");

    _file_path = std::filesystem::temp_directory_path() / "hyriseCalibration" / "csv_export" / "calibration_test.csv";
    _headers = std::vector<std::string>({"INPUT_ROWS", "OUTPUT_ROWS", "RUNTIME_NS", "SCAN_TYPE"});
  }

  CSVWriter default_writer() { return CSVWriter(_file_path, _headers); }

  ~CSVWriterTest() override {
    std::filesystem::remove_all(std::filesystem::temp_directory_path() / "hyriseCalibration");
  }

  std::filesystem::path _file_path;
  std::vector<std::string> _headers;
};

TEST_F(CSVWriterTest, DirectoryExistsAfterInitialization) {
  std::filesystem::remove(_file_path);
  const auto csv_writer = CSVWriter(_file_path, _headers);

  EXPECT_TRUE(std::filesystem::exists(_file_path.parent_path()));
  EXPECT_TRUE(std::filesystem::is_directory(_file_path.parent_path()));
}

TEST_F(CSVWriterTest, CreatesFileWithHeadersIfNotExists) {
  std::filesystem::remove(_file_path);
  const auto csv_writer = CSVWriter(_file_path, _headers);

  EXPECT_TRUE(std::filesystem::exists(_file_path));

  std::string line;
  std::ifstream f(_file_path);

  // Get first line; first line should be the header
  std::getline(f, line);

  EXPECT_EQ(line, "INPUT_ROWS,OUTPUT_ROWS,RUNTIME_NS,SCAN_TYPE");
}

TEST_F(CSVWriterTest, HeaderRemainsAfterValueAddition) {
  auto csv_writer = default_writer();

  csv_writer.set_value("INPUT_ROWS", 10);
  csv_writer.set_value("OUTPUT_ROWS", 5);
  csv_writer.set_value("RUNTIME_NS", 5000);
  csv_writer.set_value("SCAN_TYPE", "ColumnScan");

  csv_writer.write_row();

  std::string line;
  std::ifstream f(_file_path);
  std::getline(f, line);
  // Check if header is still untouched
  EXPECT_EQ(line, "INPUT_ROWS,OUTPUT_ROWS,RUNTIME_NS,SCAN_TYPE");

  // Check if row was added
  std::getline(f, line);
  EXPECT_EQ(line, "10,5,5000,ColumnScan");
}

TEST_F(CSVWriterTest, ThrowIfValueForHeaderIsAddedTwice) {
  auto csv_writer = default_writer();

  csv_writer.set_value("INPUT_ROWS", 100);
  EXPECT_ANY_THROW(csv_writer.set_value("INPUT_ROWS", 1000));
}

TEST_F(CSVWriterTest, WriteMultipleRowsToFile) {
  auto const input_rows = std::vector<int>({40, 120, 300, 4058, 3004, 1203, 5960, 420});
  auto const output_rows = std::vector<int>({39, 30, 49, 2385, 458, 394, 5960, 42});
  auto const runtime_ns = std::vector<int>({
      30403,
      23993,
      102933,
      58632,
      594,
      1230,
      49513,
  });
  auto const scan_type = std::vector<std::string>({"ReferenceScan", "ReferenceScan", "ColumnScan", "ReferenceScan",
                                                   "ColumnScan", "ColumnScan", "ColumnScan", "ReferenceScan"});

  auto csv_writer = CSVWriter(_file_path, _headers);
  constexpr auto delimiter = ',';

  uint64_t num_entries = input_rows.size();
  for (uint64_t row_index = 0; row_index < num_entries; ++row_index) {
    csv_writer.set_value("INPUT_ROWS", input_rows[row_index]);
    csv_writer.set_value("OUTPUT_ROWS", output_rows[row_index]);
    csv_writer.set_value("RUNTIME_NS", runtime_ns[row_index]);
    csv_writer.set_value("SCAN_TYPE", scan_type[row_index]);

    csv_writer.write_row();
  }

  std::string line;
  std::ifstream f(_file_path);
  std::getline(f, line);  // first line is the header; we ignore the header here

  for (uint64_t row_index = 0; row_index < num_entries; ++row_index) {
    std::stringstream ss;
    ss << input_rows[row_index] << delimiter;
    ss << output_rows[row_index] << delimiter;
    ss << runtime_ns[row_index] << delimiter;
    ss << scan_type[row_index];

    std::getline(f, line);
    EXPECT_EQ(line, ss.str());
  }
}

TEST_F(CSVWriterTest, ReplaceExistingFileIfFlagIsSet) {
  const auto csv_writer = default_writer();  // create an file with headers

  const auto new_headers = std::vector<std::string>({"COLUMN_1", "COLUMN_2", "COLUMN_3", "COLUMN_4"});
  const auto new_csv_writer = CSVWriter(_file_path, new_headers);

  std::string line;
  std::ifstream f(_file_path);
  std::getline(f, line);

  EXPECT_EQ(line, "COLUMN_1,COLUMN_2,COLUMN_3,COLUMN_4");  // first line must be the new header
}

TEST_F(CSVWriterTest, ThrowIfDelimiterInValue) {
  auto csv_writer = CSVWriter(_file_path, _headers);
  EXPECT_ANY_THROW(csv_writer.set_value("INPUT_ROWS", "434,123"));
  EXPECT_ANY_THROW(csv_writer.set_value("OUTPUT_ROWS", "39213213,"));
  EXPECT_ANY_THROW(csv_writer.set_value("RUNTIME_NS", ",434123"));
}

}  // namespace opossum
