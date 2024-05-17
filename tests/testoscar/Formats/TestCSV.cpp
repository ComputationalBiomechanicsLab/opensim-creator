#include <oscar/Formats/CSV.h>

#include <gtest/gtest.h>

#include <sstream>

using namespace osc;

TEST(read_csv_row, ReadingAnEmptyStreamReturnsASingleEmptyColumn)
{
    std::istringstream input;
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(output->size(), 1);
    ASSERT_EQ(output->at(0), "");
}

TEST(read_csv_row, ReadingAStreamContainingASpaceReturnsASingleColumnContainingTheSpace)
{
    std::istringstream input{" "};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(output->size(), 1);
    ASSERT_EQ(output->at(0), " ");
}

TEST(read_csv_row, ReadingAStreamContainingTwoCommasShouldReturnThreeEmptyColumns)
{
    std::istringstream input{",,"};
    const  std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(output->size(), 3);
    for (const std::string& column : *output) {
        ASSERT_TRUE(column.empty()) << column << " is not an empty string";
    }
}

TEST(read_csv_row, ReadingAStreamContainingStandardColumnHeadersReturnsExpectedOutput)
{
    std::istringstream input{"col1,col2,col3"};
    const std::vector<std::string> expected_output = {"col1", "col2", "col3"};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expected_output);
}

TEST(read_csv_row, ReadingAStreamContainingMultipleLinesReturnsEachRowAsExpected)
{
    std::istringstream input{"col1,col2\n1,2\n,\n \n\n"};
    const std::vector<std::vector<std::string>> expected_outputs = {
        {"col1", "col2"},
        {"1", "2"},
        {"", ""},
        {" "},
        {""},
    };

    for (const auto& expected_output : expected_outputs) {
        const std::optional<std::vector<std::string>> columns = read_csv_row(input);
        ASSERT_TRUE(columns.has_value());
        ASSERT_EQ(*columns, expected_output);
    }
}

TEST(read_csv_row, ReadingAStreamContainingNestedQuotesWorksAsExpectedForBasicExample)
{
    std::istringstream input{R"("contains spaces",col2)"};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(output->size(), 2);
    ASSERT_EQ(output->at(0), "contains spaces");
    ASSERT_EQ(output->at(1), "col2");
}

TEST(read_csv_row, ReadingAStreamWithNestedQuotesWorksAsExpectedForExampleExportedFromExcel)
{
    std::istringstream input{R"("""quoted text""",col2)"};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(output->size(), 2);
    ASSERT_EQ(output->at(0), R"("quoted text")");
    ASSERT_EQ(output->at(1), "col2");
}

TEST(read_csv_row, ReadingAStreamAfterEOFReturnsAnEmptyOptional)
{
    std::istringstream input{"col1,col2,col3"};

    const std::vector<std::string> expected_first_output = {"col1", "col2", "col3"};
    const std::optional<std::vector<std::string>> first_output = read_csv_row(input);

    ASSERT_TRUE(first_output.has_value());
    ASSERT_EQ(*first_output, expected_first_output);

    const std::optional<std::vector<std::string>> second_output_after_eof = read_csv_row(input);

    ASSERT_FALSE(second_output_after_eof.has_value());
}
TEST(read_csv_row, EdgeCase1)
{
    // e.g. https://stackoverflow.com/questions/9714322/parsing-a-csv-edge-cases

    std::istringstream input{R"(a,b"c"d,e)"};
    const std::vector<std::string> expected_output = {"a", R"(b"c"d)", "e"};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expected_output);
}

TEST(read_csv_row, EdgeCase2)
{
    // e.g. https://stackoverflow.com/questions/9714322/parsing-a-csv-edge-cases

    std::istringstream input{R"(a,"bc"d,e)"};
    const std::vector<std::string> expected_output = {"a", "bcd", "e"};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expected_output);
}

TEST(read_csv_row, EdgeCase3)
{
    // from GitHub: maxogden/csv-spectrum: comma_in_quotes.csv

    std::istringstream input{R"(John,Doe,120 any st.,"Anytown, WW",08123)"};
    const std::vector<std::string> expected_output = {"John", "Doe", "120 any st.", "Anytown, WW", "08123"};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expected_output);
}

TEST(read_csv_row, EdgeCase4)
{
    // from GitHub: maxogden/csv-spectrum: empty.csv

    std::istringstream input{R"(1,"","")"};
    const std::vector<std::string> expected_output = {"1", "", ""};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expected_output);
}

TEST(read_csv_row, EdgeCase5)
{
    // from GitHub: maxogden/csv-spectrum: empty_crlf.csv

    std::istringstream input{"1,\"\",\"\"\r\n"};
    const std::vector<std::string> expected_output = {"1", "", ""};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expected_output);
}

TEST(read_csv_row, EdgeCase6)
{
    // from GitHub: maxogden/csv-spectrum: escaped_quotes.csv

    std::istringstream input{R"(1,"ha ""ha"" ha")"};
    const std::vector<std::string> expected_output = {"1", R"(ha "ha" ha)"};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expected_output);
}

TEST(read_csv_row, EdgeCase7)
{
    // from GitHub: maxogden/csv-spectrum: json.csv

    std::istringstream input{R"(1,"{""type"": ""Point"", ""coordinates"": [102.0, 0.5]}")"};
    const std::vector<std::string> expected_output = {"1", R"({"type": "Point", "coordinates": [102.0, 0.5]})"};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expected_output);
}

TEST(read_csv_row, EdgeCase8)
{
    // from GitHub: maxogden/csv-spectrum: newlines.csv

    std::istringstream input{"\"Once upon \na time\",5,6"};
    const std::vector<std::string> expected_output = {"Once upon \na time", "5", "6"};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(output, expected_output);
}

TEST(read_csv_row, EdgeCase9)
{
    // from GitHub: maxogden/csv-spectrum: newlines_crlf.csv

    std::istringstream input{"\"Once upon \r\na time\",5,6"};
    const std::vector<std::string> expected_output = {"Once upon \r\na time", "5", "6"};
    const std::optional<std::vector<std::string>> output = read_csv_row(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expected_output);
}

TEST(read_csv_row, EdgeCase10)
{
    // from GitHub: maxogden/csv-spectrum: simple_crlf.csv

    std::istringstream input{"a,b,c\r\n1,2,3"};
    const std::vector<std::vector<std::string>> expected_outputs = {
        {"a", "b", "c"},
        {"1", "2", "3"},
    };
    for (const auto& expected_output : expected_outputs) {
        const std::optional<std::vector<std::string>> output = read_csv_row(input);
        ASSERT_TRUE(output.has_value());
        ASSERT_EQ(*output, expected_output);
    }
}

TEST(write_csv_row, WritesExpectedContentForBasicExample)
{
    const std::vector<std::string> input = {"a", "b", "c"};
    const std::string expected_output = "a,b,c\n";

    std::stringstream output;
    write_csv_row(output, input);

    ASSERT_EQ(output.str(), expected_output);
}

TEST(write_csv_row, WritesExpectedContentForMultilineExample)
{
    const std::vector<std::vector<std::string>> inputs = {
        {"col1", "col2", "col3"},
        {"a", "b", "c"},
    };
    const std::string expected_output = "col1,col2,col3\na,b,c\n";

    std::stringstream output;
    for (const auto& input : inputs) {
        write_csv_row(output, input);
    }

    ASSERT_EQ(output.str(), expected_output);
}

TEST(write_csv_row, EdgeCase1)
{
    const std::vector<std::vector<std::string>> inputs = {
        {"\"quoted column\"", "column, with comma", "nested\nnewline"},
        {"a", "b", "\"hardmode, maybe?\nwho knows"},
    };
    const std::string expected_output = "\"\"\"quoted column\"\"\",\"column, with comma\",\"nested\nnewline\"\na,b,\"\"\"hardmode, maybe?\nwho knows\"\n";

    std::stringstream output;
    for (const auto& input : inputs) {
        write_csv_row(output, input);
    }

    ASSERT_EQ(output.str(), expected_output);
}
