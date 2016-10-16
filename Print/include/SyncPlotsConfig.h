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

struct SyncPlotEntry {
    std::string mine_name, other_name;
    size_t n_bins;
    Range<double> x_range;

    SyncPlotEntry() : n_bins(0) {}
};

std::ostream& operator <<(std::ostream& s, const SyncPlotEntry& entry)
{
    static const std::string sep = " ";
    s << entry.mine_name << sep;
    if(entry.other_name.length())
        s << entry.other_name <<  sep;
    s << entry.n_bins << sep << entry.x_range;
    return s;
}

std::istream& operator >>(std::istream& s, SyncPlotEntry& entry)
{
    std::string line;
    std::getline(s, line);
    std::vector<std::string> params = ConfigEntryReader::ParseOrderedParameterList(line, true);

    try {
        if(params.size() >= 4 && params.size() <= 5) {
            size_t n = 0;
            entry.mine_name = params.at(n++);
            entry.other_name = params.size() > 4 ? params.at(n++) : entry.mine_name;
            entry.n_bins = Parse<size_t>(params.at(n++));
            const std::string range_str = params.at(n) + " " + params.at(n+1);
            entry.x_range = Parse<Range<double>>(range_str);
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
