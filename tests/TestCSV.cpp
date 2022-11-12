#include "src/Formats/CSV.hpp"

#include <gtest/gtest.h>

#include <sstream>

TEST(CSVReader, CanConstructFromStringStream)
{
    std::istringstream ss{"col1,col2"};
    osc::CSVReader reader{ss};
}

TEST(CSVReader, CanConstructWithEmptyStringStream)
{
    std::istringstream ss{};
    osc::CSVReader reader{ss};
}

TEST(CSVReader, CanMoveConstruct)
{
    std::istringstream stream{};
    osc::CSVReader a{stream};
    osc::CSVReader b{std::move(a)};
}

TEST(CSVReader, CanMoveAssign)
{
    std::istringstream stream1{};
    std::istringstream stream2{};
    osc::CSVReader a{stream1};
    osc::CSVReader b{stream2};
    a = std::move(b);
}

TEST(CSVReader, CallingNextOnEmptyStringReturnsEmptyString)
{
    std::istringstream stream1{};
    osc::CSVReader reader{stream1};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 1);
    ASSERT_EQ(rv->at(0), "");
}

TEST(CSVReader, CallingNextOnWhitespaceStringReturnsNonemptyOptional)
{
    std::istringstream stream{" "};
    osc::CSVReader reader{stream};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 1);
    ASSERT_EQ(rv->at(0), " ");
}

TEST(CSVReader, CallingNextOnStringWithEmptyColumnsReturnsEmptyStrings)
{
    std::istringstream stream{",,"};
    osc::CSVReader reader{stream};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 3);
    for (std::string const& s : *rv)
    {
        ASSERT_EQ(s, "");
    }
}

TEST(CSVReader, CallingNextOnStandardColumnHeaderStringsReturnsExpectedResult)
{
    std::istringstream stream{"col1,col2,col3"};
    osc::CSVReader reader{stream};
    std::vector<std::string> expected = {"col1", "col2", "col3"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, CallingNextOnMultilineInputReturnsExpectedResult)
{
    std::istringstream stream{"col1,col2\n1,2\n,\n \n\n"};
    osc::CSVReader reader{stream};

    std::vector<std::vector<std::string>> expected =
    {
        {"col1", "col2"},
        {"1", "2"},
        {"", ""},
        {" "},
        {""},
    };

    for (size_t i = 0; i < expected.size(); ++i)
    {
        std::optional<std::vector<std::string>> rv = reader.next();
        ASSERT_TRUE(rv.has_value());
        ASSERT_EQ(*rv, expected[i]);
    }
}

TEST(CSVReader, CallingNextWithNestedQuotesWorksAsExpectedForBasicExample)
{
    std::istringstream stream{R"("contains spaces",col2)"};
    osc::CSVReader reader{stream};
    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 2);
    ASSERT_EQ(rv->at(0), "contains spaces");
    ASSERT_EQ(rv->at(1), "col2");
}

TEST(CSVReader, CallingNextWithNestedQuotesWorksAsExpectedExcelExample)
{
    std::istringstream stream{R"("""quoted text""",col2)"};
    osc::CSVReader reader{stream};
    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 2);
    ASSERT_EQ(rv->at(0), R"("quoted text")");
    ASSERT_EQ(rv->at(1), "col2");
}

TEST(CSVReader, CallingNextAfterEOFReturnsEmptyOptional)
{
    std::istringstream stream{"col1,col2,col3"};
    osc::CSVReader reader{stream};
    std::vector<std::string> expected = {"col1", "col2", "col3"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);

    std::optional<std::vector<std::string>> rv2 = reader.next();

    ASSERT_FALSE(rv2.has_value());
}
TEST(CSVReader, EdgeCase1)
{
    // e.g. https://stackoverflow.com/questions/9714322/parsing-a-csv-edge-cases

    std::istringstream stream{R"(a,b"c"d,e)"};
    osc::CSVReader reader{stream};
    std::vector<std::string> expected = {"a", R"(b"c"d)", "e"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase2)
{
    // e.g. https://stackoverflow.com/questions/9714322/parsing-a-csv-edge-cases

    std::istringstream stream{R"(a,"bc"d,e)"};
    osc::CSVReader reader{stream};
    std::vector<std::string> const expected = {"a", "bcd", "e"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase3)
{
    // from GitHub: maxogden/csv-spectrum: comma_in_quotes.csv

    std::istringstream stream{R"(John,Doe,120 any st.,"Anytown, WW",08123)"};
    osc::CSVReader reader{stream};
    std::vector<std::string> const expected = {"John", "Doe", "120 any st.", "Anytown, WW", "08123"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase4)
{
    // from GitHub: maxogden/csv-spectrum: empty.csv

    std::istringstream stream{R"(1,"","")"};
    osc::CSVReader reader{stream};
    std::vector<std::string> const expected = {"1", "", ""};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase5)
{
    // from GitHub: maxogden/csv-spectrum: empty_crlf.csv

    std::istringstream stream{"1,\"\",\"\"\r\n"};
    osc::CSVReader reader{stream};
    std::vector<std::string> const expected = {"1", "", ""};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase6)
{
    // from GitHub: maxogden/csv-spectrum: escaped_quotes.csv

    std::istringstream stream{R"(1,"ha ""ha"" ha")"};
    osc::CSVReader reader{stream};
    std::vector<std::string> const expected = {"1", R"(ha "ha" ha)"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase7)
{
    // from GitHub: maxogden/csv-spectrum: json.csv

    std::istringstream stream{R"(1,"{""type"": ""Point"", ""coordinates"": [102.0, 0.5]}")"};
    osc::CSVReader reader{stream};
    std::vector<std::string> const expected = {"1", R"({"type": "Point", "coordinates": [102.0, 0.5]})"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase8)
{
    // from GitHub: maxogden/csv-spectrum: newlines.csv

    std::istringstream stream{"\"Once upon \na time\",5,6"};
    osc::CSVReader reader{stream};
    std::vector<std::string> const expected = {"Once upon \na time", "5", "6"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase9)
{
    // from GitHub: maxogden/csv-spectrum: newlines_crlf.csv

    std::istringstream stream{"\"Once upon \r\na time\",5,6"};
    osc::CSVReader reader{stream};
    std::vector<std::string> const expected = {"Once upon \r\na time", "5", "6"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase10)
{
    // from GitHub: maxogden/csv-spectrum: simple_crlf.csv

    std::istringstream stream{"a,b,c\r\n1,2,3"};
    osc::CSVReader reader{stream};

    std::vector<std::vector<std::string>> const expected =
    {
        {"a", "b", "c"},
        {"1", "2", "3"},
    };

    for (size_t i = 0; i < expected.size(); ++i)
    {
        std::optional<std::vector<std::string>> rv = reader.next();
        ASSERT_TRUE(rv.has_value());
        ASSERT_EQ(*rv, expected[i]);
    }
}

TEST(CSVWriter, CanBeConstructedFromStringStream)
{
    std::stringstream stream;
    osc::CSVWriter writer{stream};
}

TEST(CSVWriter, CanBeMoveConstructed)
{
    std::stringstream stream;

    osc::CSVWriter a{stream};
    osc::CSVWriter b{std::move(a)};
}

TEST(CSVWriter, CanBeMoveAssigned)
{
    std::stringstream stream1;
    osc::CSVWriter a{stream1};

    std::stringstream stream2;
    osc::CSVWriter b{stream2};

    b = std::move(a);
}

TEST(CSVWriter, WriteRowWritesExpectedContentForBasicExample)
{
    std::stringstream stream;
    osc::CSVWriter writer{stream};

    std::vector<std::string> input = {"a", "b", "c"};
    std::string expectedOutput = "a,b,c\n";

    writer.writeRow(input);

    ASSERT_EQ(stream.str(), expectedOutput);
}

TEST(CSVWriter, WriteRowWritesExpectedContentForMultilineExample)
{
    std::stringstream stream;
    osc::CSVWriter writer{stream};

    std::vector<std::vector<std::string>> inputs =
    {
        {"col1", "col2", "col3"},
        {"a", "b", "c"},
    };
    std::string expectedOutput = "col1,col2,col3\na,b,c\n";

    for (std::vector<std::string> const& input : inputs)
    {
        writer.writeRow(input);
    }

    ASSERT_EQ(stream.str(), expectedOutput);
}

TEST(CSVWriter, EdgeCase1)
{
    std::stringstream stream;
    osc::CSVWriter writer{stream};

    std::vector<std::vector<std::string>> inputs =
    {
        {"\"quoted column\"", "column, with comma", "nested\nnewline"},
        {"a", "b", "\"hardmode, maybe?\nwho knows"},
    };
    std::string expectedOutput = "\"\"\"quoted column\"\"\",\"column, with comma\",\"nested\nnewline\"\na,b,\"\"\"hardmode, maybe?\nwho knows\"\n";

    for (std::vector<std::string> const& input : inputs)
    {
        writer.writeRow(input);
    }

    ASSERT_EQ(stream.str(), expectedOutput);
}