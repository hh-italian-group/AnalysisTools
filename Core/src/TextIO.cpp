/*! Definition of primitives for a text based input/output.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include "AnalysisTools/Core/include/TextIO.h"

#include <cmath>
#include <iomanip>
#include <unordered_set>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/convenience.hpp>
#include "AnalysisTools/Core/include/exception.h"

namespace analysis {

std::string RemoveFileExtension(const std::string& file_name)
{
    return boost::filesystem::change_extension(file_name, "").string();
}

std::string GetFileNameWithoutPath(const std::string& file_name)
{
    const size_t lastindex = file_name.find_last_of("/");
    if(lastindex == std::string::npos)
        return file_name;
    else
        return file_name.substr(lastindex+1);
}

std::vector<std::string> SplitValueList(const std::string& _values_str, bool allow_duplicates,
                                        const std::string& separators, bool enable_token_compress)
{
    std::string values_str = _values_str;
    std::vector<std::string> result;
    if(enable_token_compress)
        boost::trim_if(values_str, boost::is_any_of(separators));
    if(!values_str.size()) return result;
    const auto token_compress = enable_token_compress ? boost::algorithm::token_compress_on
                                                      : boost::algorithm::token_compress_off;
    boost::split(result, values_str, boost::is_any_of(separators), token_compress);
    if(!allow_duplicates) {
        std::unordered_set<std::string> set_result;
        for(const std::string& value : result) {
            if(set_result.count(value))
                throw exception("Value '%1%' listed more than once in the value list '%2%'.") % value % values_str;
            set_result.insert(value);
        }
    }
    return result;
}

std::vector<std::string> ReadValueList(std::istream& stream, size_t number_of_items, bool allow_duplicates,
                                       const std::string& separators, bool enable_token_compress)
{
    const auto stream_exceptions = stream.exceptions();
    stream.exceptions(std::istream::goodbit);
    try {
        std::vector<std::string> result;
        std::unordered_set<std::string> set_result;
        const auto predicate = boost::is_any_of(separators);
        size_t n = 0;
        for(; n < number_of_items; ++n) {
            std::string value;
            while(true) {
                const auto c = stream.get();
                if(!stream.good()) {
                    if(stream.eof()) break;
                    throw exception("Failed to read values from stream.");
                }
                if(predicate(c)) {
                    if(!value.size() && enable_token_compress) continue;
                    break;
                }
                value.push_back(static_cast<char>(c));
            }
            if(!allow_duplicates && set_result.count(value))
                throw exception("Value '%1%' listed more than once in the input stream.") % value;
            result.push_back(value);
            set_result.insert(value);
        }
        if(n != number_of_items)
            throw exception("Expected %1% items, while read only %2%.") % number_of_items % n;

        stream.clear();
        stream.exceptions(stream_exceptions);
        return result;
    } catch(exception&) {
        stream.clear();
        stream.exceptions(stream_exceptions);
        throw;
    }
}

} // namespace analysis
