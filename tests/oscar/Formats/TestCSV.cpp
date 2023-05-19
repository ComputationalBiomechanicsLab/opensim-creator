#include "oscar/Formats/CSV.hpp"

#include <gtest/gtest.h>

#include <sstream>

TEST(CSVReader, CanConstructFromStringStream)
{
    auto ss = std::make_shared<std::istringstream>("col1,col2");
    osc::CSVReader reader{ss};
}

TEST(CSVReader, CanConstructWithEmptyStringStream)
{
    auto ss = std::make_shared<std::istringstream>();
    osc::CSVReader reader{ss};
}

TEST(CSVReader, CanMoveConstruct)
{
    auto ss = std::make_shared<std::istringstream>();
    osc::CSVReader a{ss};
    osc::CSVReader b{std::move(a)};
}

TEST(CSVReader, CanMoveAssign)
{
    auto ss1 = std::make_shared<std::istringstream>();
    auto ss2 = std::make_shared<std::istringstream>();
    osc::CSVReader a{ss1};
    osc::CSVReader b{ss2};
    a = std::move(b);
}

TEST(CSVReader, CallingNextOnEmptyStringReturnsEmptyString)
{
    auto ss = std::make_shared<std::istringstream>();
    osc::CSVReader reader{ss};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 1);
    ASSERT_EQ(rv->at(0), "");
}

TEST(CSVReader, CallingNextOnWhitespaceStringReturnsNonemptyOptional)
{
    auto ss = std::make_shared<std::istringstream>(" ");
    osc::CSVReader reader{ss};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 1);
    ASSERT_EQ(rv->at(0), " ");
}

TEST(CSVReader, CallingNextOnStringWithEmptyColumnsReturnsEmptyStrings)
{
    auto ss = std::make_shared<std::istringstream>(",,");
    osc::CSVReader reader{ss};

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
    auto ss = std::make_shared<std::istringstream>("col1,col2,col3");
    osc::CSVReader reader{ss};
    std::vector<std::string> expected = {"col1", "col2", "col3"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, CallingNextOnMultilineInputReturnsExpectedResult)
{
    auto ss = std::make_shared<std::istringstream>("col1,col2\n1,2\n,\n \n\n");
    osc::CSVReader reader{ss};

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
    auto ss = std::make_shared<std::istringstream>(R"("contains spaces",col2)");
    osc::CSVReader reader{ss};
    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 2);
    ASSERT_EQ(rv->at(0), "contains spaces");
    ASSERT_EQ(rv->at(1), "col2");
}

TEST(CSVReader, CallingNextWithNestedQuotesWorksAsExpectedExcelExample)
{
    auto ss = std::make_shared<std::istringstream>(R"("""quoted text""",col2)");
    osc::CSVReader reader{ss};
    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(rv->size(), 2);
    ASSERT_EQ(rv->at(0), R"("quoted text")");
    ASSERT_EQ(rv->at(1), "col2");
}

TEST(CSVReader, CallingNextAfterEOFReturnsEmptyOptional)
{
    auto ss = std::make_shared<std::istringstream>("col1,col2,col3");
    osc::CSVReader reader{ss};
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

    auto ss = std::make_shared<std::istringstream>(R"(a,b"c"d,e)");
    osc::CSVReader reader{ss};
    std::vector<std::string> expected = {"a", R"(b"c"d)", "e"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase2)
{
    // e.g. https://stackoverflow.com/questions/9714322/parsing-a-csv-edge-cases

    auto ss = std::make_shared<std::istringstream>(R"(a,"bc"d,e)");
    osc::CSVReader reader{ss};
    std::vector<std::string> const expected = {"a", "bcd", "e"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase3)
{
    // from GitHub: maxogden/csv-spectrum: comma_in_quotes.csv

    auto ss = std::make_shared<std::istringstream>(R"(John,Doe,120 any st.,"Anytown, WW",08123)");
    osc::CSVReader reader{ss};
    std::vector<std::string> const expected = {"John", "Doe", "120 any st.", "Anytown, WW", "08123"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase4)
{
    // from GitHub: maxogden/csv-spectrum: empty.csv

    auto ss = std::make_shared<std::istringstream>(R"(1,"","")");
    osc::CSVReader reader{ss};
    std::vector<std::string> const expected = {"1", "", ""};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase5)
{
    // from GitHub: maxogden/csv-spectrum: empty_crlf.csv

    auto ss = std::make_shared<std::istringstream>("1,\"\",\"\"\r\n");
    osc::CSVReader reader{ss};
    std::vector<std::string> const expected = {"1", "", ""};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase6)
{
    // from GitHub: maxogden/csv-spectrum: escaped_quotes.csv

    auto ss = std::make_shared<std::istringstream>(R"(1,"ha ""ha"" ha")");
    osc::CSVReader reader{ss};
    std::vector<std::string> const expected = {"1", R"(ha "ha" ha)"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase7)
{
    // from GitHub: maxogden/csv-spectrum: json.csv

    auto ss = std::make_shared<std::istringstream>(R"(1,"{""type"": ""Point"", ""coordinates"": [102.0, 0.5]}")");
    osc::CSVReader reader{ss};
    std::vector<std::string> const expected = {"1", R"({"type": "Point", "coordinates": [102.0, 0.5]})"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase8)
{
    // from GitHub: maxogden/csv-spectrum: newlines.csv

    auto ss = std::make_shared<std::istringstream>("\"Once upon \na time\",5,6");
    osc::CSVReader reader{ss};
    std::vector<std::string> const expected = {"Once upon \na time", "5", "6"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase9)
{
    // from GitHub: maxogden/csv-spectrum: newlines_crlf.csv

    auto ss = std::make_shared<std::istringstream>("\"Once upon \r\na time\",5,6");
    osc::CSVReader reader{ss};
    std::vector<std::string> const expected = {"Once upon \r\na time", "5", "6"};

    std::optional<std::vector<std::string>> rv = reader.next();

    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(*rv, expected);
}

TEST(CSVReader, EdgeCase10)
{
    // from GitHub: maxogden/csv-spectrum: simple_crlf.csv

    auto ss = std::make_shared<std::istringstream>("a,b,c\r\n1,2,3");
    osc::CSVReader reader{ss};

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
    auto ss = std::make_shared<std::stringstream>();
    osc::CSVWriter writer{ss};
}

TEST(CSVWriter, CanBeMoveConstructed)
{
    auto ss = std::make_shared<std::stringstream>();

    osc::CSVWriter a{ss};
    osc::CSVWriter b{std::move(a)};
}

TEST(CSVWriter, CanBeMoveAssigned)
{
    auto ss1 = std::make_shared<std::stringstream>();
    osc::CSVWriter a{ss1};

    auto ss2 = std::make_shared<std::stringstream>();
    osc::CSVWriter b{ss2};

    b = std::move(a);
}

TEST(CSVWriter, WriteRowWritesExpectedContentForBasicExample)
{
    auto ss = std::make_shared<std::stringstream>();
    osc::CSVWriter writer{ss};

    std::vector<std::string> input = {"a", "b", "c"};
    std::string expectedOutput = "a,b,c\n";

    writer.writeRow(input);

    ASSERT_EQ(ss->str(), expectedOutput);
}

TEST(CSVWriter, WriteRowWritesExpectedContentForMultilineExample)
{
    auto ss = std::make_shared<std::stringstream>();
    osc::CSVWriter writer{ss};

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

    ASSERT_EQ(ss->str(), expectedOutput);
}

TEST(CSVWriter, EdgeCase1)
{
    auto ss = std::make_shared<std::stringstream>();
    osc::CSVWriter writer{ss};

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

    ASSERT_EQ(ss->str(), expectedOutput);
}