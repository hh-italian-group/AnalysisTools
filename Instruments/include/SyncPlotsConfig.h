/*! Configuration EventSync.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include "AnalysisTools/Core/include/EnumNameMap.h"
#include "AnalysisTools/Core/include/NumericPrimitives.h"

namespace analysis {

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
    Condition();

    bool pass_int(int value) const;
    bool pass_double(double value) const;

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

std::ostream& operator <<(std::ostream& s, const Condition& cond);
std::istream& operator >>(std::istream& s, Condition& cond);

struct SyncPlotEntry {
    static constexpr size_t N = 2;

    std::array<std::string, N> names;
    size_t n_bins;
    Range<double> x_range;
    std::array<Condition, N> conditions;

    bool HasAtLeastOneCondition() const;
    SyncPlotEntry();
};

std::ostream& operator <<(std::ostream& s, const SyncPlotEntry& entry);
std::istream& operator >>(std::istream& s, SyncPlotEntry& entry);

class SyncPlotConfig {
public:
    static constexpr size_t N = SyncPlotEntry::N;

    explicit SyncPlotConfig(const std::string& file_name);
    const std::vector<std::string>& GetIdBranches(size_t n) const;
    const std::vector<SyncPlotEntry>& GetEntries() const;

private:
    std::vector<std::string> ReadIdBranches(std::ifstream& cfg);

private:
    std::array<std::vector<std::string>, N> idBranches;
    std::vector<SyncPlotEntry> entries;
};

} // namespace analysis
