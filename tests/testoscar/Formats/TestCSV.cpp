#include <oscar/Formats/CSV.hpp>

#include <gtest/gtest.h>

#include <sstream>

TEST(ReadCSVRow, CallingReadCSVRowOnEmptyStringReturnsEmptyString)
{
    std::istringstream input;
    std::optional<std::vector<std::string>> const rv = osc::ReadCSVRow(input);

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 1);
    ASSERT_EQ(rv->at(0), "");
}

TEST(CSVReader, CallingNextOnWhitespaceStringReturnsNonemptyOptional)
{
    std::istringstream input{" "};
    std::optional<std::vector<std::string>> const rv = osc::ReadCSVRow(input);

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 1);
    ASSERT_EQ(rv->at(0), " ");
}

TEST(CSVReader, CallingNextOnStringWithEmptyColumnsReturnsEmptyStrings)
{
    std::istringstream input{",,"};
    std::optional<std::vector<std::string>> const rv = osc::ReadCSVRow(input);

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 3);
    for (std::string const& s : *rv)
    {
        ASSERT_EQ(s, "");
    }
}

TEST(CSVReader, CallingNextOnStandardColumnHeaderStringsReturnsExpectedResult)
{
    std::istringstream input{"col1,col2,col3"};
    std::vector<std::string> const expectedOutput = {"col1", "col2", "col3"};
    std::optional<std::vector<std::string>> const rv = osc::ReadCSVRow(input);

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expectedOutput);
}

TEST(CSVReader, CallingNextOnMultilineInputReturnsExpectedResult)
{
    std::istringstream input{"col1,col2\n1,2\n,\n \n\n"};
    std::vector<std::vector<std::string>> const expectedOutputs =
    {
        {"col1", "col2"},
        {"1", "2"},
        {"", ""},
        {" "},
        {""},
    };

    for (auto const& expectedOutput : expectedOutputs)
    {
        std::optional<std::vector<std::string>> const rv = osc::ReadCSVRow(input);
        ASSERT_TRUE(rv.has_value());
        ASSERT_EQ(*rv, expectedOutput);
    }
}

TEST(CSVReader, CallingNextWithNestedQuotesWorksAsExpectedForBasicExample)
{
    std::istringstream input{R"("contains spaces",col2)"};
    std::optional<std::vector<std::string>> const rv = osc::ReadCSVRow(input);

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 2);
    ASSERT_EQ(rv->at(0), "contains spaces");
    ASSERT_EQ(rv->at(1), "col2");
}

TEST(CSVReader, CallingNextWithNestedQuotesWorksAsExpectedExcelExample)
{
    std::istringstream input{R"("""quoted text""",col2)"};
    std::optional<std::vector<std::string>> const rv = osc::ReadCSVRow(input);

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 2);
    ASSERT_EQ(rv->at(0), R"("quoted text")");
    ASSERT_EQ(rv->at(1), "col2");
}

TEST(CSVReader, CallingNextAfterEOFReturnsEmptyOptional)
{
    std::istringstream input{"col1,col2,col3"};
    std::vector<std::string> const expectedFirstRow = {"col1", "col2", "col3"};

    std::optional<std::vector<std::string>> const rv1 = osc::ReadCSVRow(input);

    ASSERT_TRUE(rv1.has_value());
    ASSERT_EQ(*rv1, expectedFirstRow);

    std::optional<std::vector<std::string>> rv2 = osc::ReadCSVRow(input);

    ASSERT_FALSE(rv2.has_value());
}
TEST(CSVReader, EdgeCase1)
{
    // e.g. https://stackoverflow.com/questions/9714322/parsing-a-csv-edge-cases

    std::istringstream input{R"(a,b"c"d,e)"};
    std::vector<std::string> const expectedOutput = {"a", R"(b"c"d)", "e"};
    std::optional<std::vector<std::string>> const output = osc::ReadCSVRow(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expectedOutput);
}

TEST(CSVReader, EdgeCase2)
{
    // e.g. https://stackoverflow.com/questions/9714322/parsing-a-csv-edge-cases

    std::istringstream input{R"(a,"bc"d,e)"};
    std::vector<std::string> const expectedOutput = {"a", "bcd", "e"};
    std::optional<std::vector<std::string>> const output = osc::ReadCSVRow(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expectedOutput);
}

TEST(CSVReader, EdgeCase3)
{
    // from GitHub: maxogden/csv-spectrum: comma_in_quotes.csv

    std::istringstream input{R"(John,Doe,120 any st.,"Anytown, WW",08123)"};
    std::vector<std::string> const expectedOutput = {"John", "Doe", "120 any st.", "Anytown, WW", "08123"};
    std::optional<std::vector<std::string>> const output = osc::ReadCSVRow(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expectedOutput);
}

TEST(CSVReader, EdgeCase4)
{
    // from GitHub: maxogden/csv-spectrum: empty.csv

    std::istringstream input{R"(1,"","")"};
    std::vector<std::string> const expectedOutput = {"1", "", ""};
    std::optional<std::vector<std::string>> const output = osc::ReadCSVRow(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expectedOutput);
}

TEST(CSVReader, EdgeCase5)
{
    // from GitHub: maxogden/csv-spectrum: empty_crlf.csv

    std::istringstream input{"1,\"\",\"\"\r\n"};
    std::vector<std::string> const expectedOutput = {"1", "", ""};
    std::optional<std::vector<std::string>> const output = osc::ReadCSVRow(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expectedOutput);
}

TEST(CSVReader, EdgeCase6)
{
    // from GitHub: maxogden/csv-spectrum: escaped_quotes.csv

    std::istringstream input{R"(1,"ha ""ha"" ha")"};
    std::vector<std::string> const expectedOutput = {"1", R"(ha "ha" ha)"};
    std::optional<std::vector<std::string>> const output = osc::ReadCSVRow(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expectedOutput);
}

TEST(CSVReader, EdgeCase7)
{
    // from GitHub: maxogden/csv-spectrum: json.csv

    std::istringstream input{R"(1,"{""type"": ""Point"", ""coordinates"": [102.0, 0.5]}")"};
    std::vector<std::string> const expectedOutput = {"1", R"({"type": "Point", "coordinates": [102.0, 0.5]})"};
    std::optional<std::vector<std::string>> const output = osc::ReadCSVRow(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expectedOutput);
}

TEST(CSVReader, EdgeCase8)
{
    // from GitHub: maxogden/csv-spectrum: newlines.csv

    std::istringstream input{"\"Once upon \na time\",5,6"};
    std::vector<std::string> const expectedOutput = {"Once upon \na time", "5", "6"};
    std::optional<std::vector<std::string>> const output = osc::ReadCSVRow(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(output, expectedOutput);
}

TEST(CSVReader, EdgeCase9)
{
    // from GitHub: maxogden/csv-spectrum: newlines_crlf.csv

    std::istringstream input{"\"Once upon \r\na time\",5,6"};
    std::vector<std::string> const expectedOutput = {"Once upon \r\na time", "5", "6"};
    std::optional<std::vector<std::string>> const output = osc::ReadCSVRow(input);

    ASSERT_TRUE(output.has_value());
    ASSERT_EQ(*output, expectedOutput);
}

TEST(CSVReader, EdgeCase10)
{
    // from GitHub: maxogden/csv-spectrum: simple_crlf.csv

    std::istringstream input{"a,b,c\r\n1,2,3"};
    std::vector<std::vector<std::string>> const expectedOutput =
    {
        {"a", "b", "c"},
        {"1", "2", "3"},
    };
    for (auto const& row : expectedOutput)
    {
        std::optional<std::vector<std::string>> const rv = osc::ReadCSVRow(input);
        ASSERT_TRUE(rv.has_value());
        ASSERT_EQ(*rv, row);
    }
}

TEST(CSVWriter, WriteRowWritesExpectedContentForBasicExample)
{
    std::vector<std::string> const input = {"a", "b", "c"};
    std::string const expectedOutput = "a,b,c\n";

    std::stringstream output;
    osc::WriteCSVRow(output, input);

    ASSERT_EQ(output.str(), expectedOutput);
}

TEST(CSVWriter, WriteRowWritesExpectedContentForMultilineExample)
{
    std::vector<std::vector<std::string>> const inputs =
    {
        {"col1", "col2", "col3"},
        {"a", "b", "c"},
    };
    std::string const expectedOutput = "col1,col2,col3\na,b,c\n";

    std::stringstream output;
    for (auto const& input : inputs)
    {
        osc::WriteCSVRow(output, input);
    }

    ASSERT_EQ(output.str(), expectedOutput);
}

TEST(CSVWriter, EdgeCase1)
{
    std::vector<std::vector<std::string>> const inputs =
    {
        {"\"quoted column\"", "column, with comma", "nested\nnewline"},
        {"a", "b", "\"hardmode, maybe?\nwho knows"},
    };
    std::string const expectedOutput = "\"\"\"quoted column\"\"\",\"column, with comma\",\"nested\nnewline\"\na,b,\"\"\"hardmode, maybe?\nwho knows\"\n";

    std::stringstream output;
    for (std::vector<std::string> const& input : inputs)
    {
        osc::WriteCSVRow(output, input);
    }

    ASSERT_EQ(output.str(), expectedOutput);
}
