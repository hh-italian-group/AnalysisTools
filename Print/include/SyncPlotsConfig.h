/*! Configuration Print_SyncPlots.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <vector>
#include <fstream>
#include <numeric>
#include <boost/algorithm/string.hpp>
#include "AnalysisTools/Core/include/ConfigReader.h"
#include "AnalysisTools/Core/include/NumericPrimitives.h"

namespace analysis {

struct EventIdentifier {
    using IdType = unsigned long long;

    IdType runId, lumiBlock, eventId;

    static const EventIdentifier& Undef_event() {
        static const EventIdentifier undef_event;
        return undef_event;
    }

    EventIdentifier() :
        runId(std::numeric_limits<IdType>::max()), lumiBlock(std::numeric_limits<IdType>::max()),
        eventId(std::numeric_limits<IdType>::max()) {}

    EventIdentifier(IdType _runId, IdType _lumiBlock, IdType _eventId) :
        runId(_runId), lumiBlock(_lumiBlock), eventId(_eventId) {}

    bool operator == (const EventIdentifier& other) const { return !(*this != other); }

    bool operator != (const EventIdentifier& other) const
    {
        return runId != other.runId || lumiBlock != other.lumiBlock || eventId != other.eventId;
    }

    bool operator < (const EventIdentifier& other) const
    {
        if(runId != other.runId) return runId < other.runId;
        if(lumiBlock != other.lumiBlock) return lumiBlock < other.lumiBlock;
        return eventId < other.eventId;
    }
};

std::ostream& operator <<(std::ostream& s, const EventIdentifier& event)
{
    s << boost::format("(run, lumi, evt) = (%1%, %2%, %3%)") % event.runId % event.lumiBlock % event.eventId;
    return s;
}

enum class CondExpr { Less, More, Equal, LessOrEqual, MoreOrEqual };
ENUM_NAMES(CondExpr) = {
    { CondExpr::Less, "<" }, { CondExpr::More, ">" }, { CondExpr::Equal, "==" },
    { CondExpr::LessOrEqual, "<=" }, { CondExpr::MoreOrEqual, ">=" },
};

struct Condition {
    bool always_true;
    std::string entry;
    CondExpr expr;
    bool is_integer;
    int value_int;
    double value_double;
    Condition() : always_true(true), is_integer(true) {}

    bool pass_int(int value) const { return pass_ex(value, value_int); }
    bool pass_double(double value) const { return pass_ex(value, value_double); }

private:
    template<typename T>
    bool pass_ex(T value, T cut_value) const
    {
        if(always_true) return true;
        if(expr == CondExpr::Less) return value < cut_value;
        if(expr == CondExpr::More) return value > cut_value;
        if(expr == CondExpr::Equal) return value == cut_value;
        if(expr == CondExpr::LessOrEqual) return value <= cut_value;
        return value >= cut_value;
    }
};

std::ostream& operator <<(std::ostream& s, const Condition& cond)
{
    if(cond.always_true)
        s << 1;
    else {
        s << cond.entry << cond.expr;
        if(cond.is_integer)
            s << cond.value_int;
        else
            s << cond.value_double;
    }
    return s;
}

std::istream& operator >>(std::istream& s, Condition& cond)
{
    static const std::set<char> expr_chars = { '<', '>', '=' };
    static const std::set<char> float_chars = { '.' };
    static const std::set<char> stop_chars = { ' ', '\t' };

    bool first = true;
    cond.entry = "";
    char c;
    while(s.good()) {
        s >> c;
        if(first && c == '1') {
            cond.always_true = true;
            return s;
        } else
            cond.always_true = false;
        first = false;
        if(expr_chars.count(c)) break;
        cond.entry.push_back(c);
    }

    std::stringstream ss_expr;
    std::stringstream ss_value;
    cond.is_integer = true;
    ss_expr << c;
    s >> c;
    if(expr_chars.count(c))
        ss_expr << c;
    else {
        cond.is_integer &= !float_chars.count(c);
        ss_value << c;
    }
    ss_expr >> cond.expr;

    while(s.good()) {
        s >> c;
        if(stop_chars.count(c)) break;
        cond.is_integer &= !float_chars.count(c);
        if(!s.fail())
            ss_value << c;
    }

    if(cond.is_integer)
        ss_value >> cond.value_int;
    else
        ss_value >> cond.value_double;

    return s;
}


struct SyncPlotEntry {
    std::string my_name, other_name;
    size_t n_bins;
    Range<double> x_range;
    Condition my_condition, other_condition;

    SyncPlotEntry() : n_bins(0) {}
};

std::ostream& operator <<(std::ostream& s, const SyncPlotEntry& entry)
{
    static const std::string sep = " ";
    s << entry.my_name << sep;
    if(entry.other_name.size())
        s << entry.other_name <<  sep;
    s << entry.n_bins << sep << entry.x_range;
    if(!entry.my_condition.always_true || !entry.other_condition.always_true)
        s << sep << entry.my_condition << sep << entry.other_condition;
    return s;
}

std::istream& operator >>(std::istream& s, SyncPlotEntry& entry)
{
    std::string line;
    std::getline(s, line);
    std::vector<std::string> params = ConfigEntryReader::ParseOrderedParameterList(line, true);

    try {
        if(params.size() >= 4 && params.size() <= 6) {
            size_t n = 0;
            entry.my_name = params.at(n++);

            if(TryParse(params.at(n), entry.n_bins)) {
                entry.other_name = entry.my_name;
                ++n;
            } else {
                entry.other_name = params.at(n++);
                entry.n_bins = Parse<size_t>(params.at(n++));
            }
            const std::string range_str = params.at(n) + " " + params.at(n+1);
            entry.x_range = Parse<Range<double>>(range_str);
            n += 2;
            if(n < params.size()) {
                std::cout << "Parsing condition: " << params.at(n) << std::endl;
                std::istringstream ss(params.at(n++));
                ss >> entry.my_condition;
                std::cout << entry.my_condition;
            }
            if(n < params.size()) {
                std::istringstream ss(params.at(n++));
                ss >> entry.other_condition;
            }

            return s;
        }
    } catch(exception&) {}

    throw exception("Invalid plot entry '%1%'") % line;
}

class SyncPlotConfig {
public:
    explicit SyncPlotConfig(const std::string& file_name)
    {
        std::ifstream cfg(file_name);

        myIdBranches = ReadIdBranches(cfg);
        otherIdBranches = ReadIdBranches(cfg);

        while(cfg.good()) {
            std::string cfgLine;
            std::getline(cfg, cfgLine);
            if (!cfgLine.size() || (cfgLine.size() && cfgLine.at(0) == '#')) continue;
            SyncPlotEntry entry;
            std::istringstream ss(cfgLine);
            ss >> entry;
            entries.push_back(entry);
        }
    }

    const std::vector<std::string>& GetMyIdBranches() const { return myIdBranches; }
    const std::vector<std::string>& GetOtherIdBranches() const { return otherIdBranches; }
    const std::vector<SyncPlotEntry>& GetEntries() const { return entries; }

private:
    std::vector<std::string> ReadIdBranches(std::ifstream& cfg)
    {
        std::string cfgLine;
        std::getline(cfg, cfgLine);
        auto idBranches = ConfigEntryReader::ParseOrderedParameterList(cfgLine, false);
        if(idBranches.size() != 3)
            throw exception("Invalid event id branches line '%1%'.") % cfgLine;
        return idBranches;
    }

private:
    std::vector<std::string> myIdBranches, otherIdBranches;
    std::vector<SyncPlotEntry> entries;
};

} // namespace analysis
