/*! Configuration EventSync.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include "AnalysisTools/Instruments/include/SyncPlotsConfig.h"

#include <fstream>
#include <set>

namespace analysis {

Condition::Condition() : always_true(true), is_integer(true) {}
bool Condition::pass_int(int value) const { return pass_ex(value, value_int); }
bool Condition::pass_double(double value) const { return pass_ex(value, value_double); }

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
    char c = '\0';
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

bool SyncPlotEntry::HasAtLeastOneCondition() const
{
    for(size_t n = 0; n < N; ++n) {
        if(!conditions[n].always_true)
            return true;
    }
    return false;
}

SyncPlotEntry::SyncPlotEntry() : n_bins(0) {}

std::ostream& operator <<(std::ostream& s, const SyncPlotEntry& entry)
{
    static constexpr char sep = ' ';
    for(size_t n = 0; n < SyncPlotEntry::N; ++n) {
        if(entry.names[n].size())
            s << entry.names[n] <<  sep;
    }
    s << entry.n_bins << sep << entry.x_range;
    if(entry.HasAtLeastOneCondition()) {
        for(size_t n = 0; n < SyncPlotEntry::N; ++n)
            s << sep << entry.conditions[n];
    }
    return s;
}

std::istream& operator >>(std::istream& s, SyncPlotEntry& entry)
{
    std::string line;
    std::getline(s, line);
    const auto params = SplitValueList(line, true);

    try {
        if(params.size() >= 4 && params.size() <= 7) {
            size_t n = 0;
            entry.names[0] = params.at(n++);

            if(TryParse(params.at(n), entry.n_bins)) {
                entry.names[1] = entry.names[0];
                ++n;
            } else {
                entry.names[1] = params.at(n++);
                entry.n_bins = Parse<size_t>(params.at(n++));
            }
            const std::string range_str = params.at(n) + " " + params.at(n+1);
            entry.x_range = Parse<Range<double>>(range_str);
            n += 2;
            if(n < params.size()) {
                std::istringstream ss(params.at(n++));
                ss >> entry.conditions[0];
            }
            if(n < params.size()) {
                std::istringstream ss(params.at(n++));
                ss >> entry.conditions[1];
            }

            return s;
        }
    } catch(exception& e) { std::cerr << "ERROR: " << e.what() << std::endl; }

    throw exception("Invalid plot entry '%1%'.") % line;
}

SyncPlotConfig::SyncPlotConfig(const std::string& file_name)
{
    std::ifstream cfg(file_name);

    for(size_t n = 0; n < N; ++n)
        idBranches[n] = ReadIdBranches(cfg);

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

const std::vector<std::string>& SyncPlotConfig::GetIdBranches(size_t n) const
{
    if(n >= N)
        throw exception("Invalid id branches index.");
    return idBranches[n];
}

const std::vector<SyncPlotEntry>& SyncPlotConfig::GetEntries() const { return entries; }

std::vector<std::string> SyncPlotConfig::ReadIdBranches(std::ifstream& cfg)
{
    std::string cfgLine;
    std::getline(cfg, cfgLine);
    auto idBranches = SplitValueList(cfgLine, false);
    if(idBranches.size() < 3 || idBranches.size() > 4)
        throw exception("Invalid event id branches line '%1%'.") % cfgLine;
    return idBranches;
}

} // namespace analysis
