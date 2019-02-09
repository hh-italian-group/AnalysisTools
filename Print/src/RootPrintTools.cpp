/*! Contains useful functions to print histograms with ROOT.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include "AnalysisTools/Print/include/RootPrintTools.h"

namespace root_ext {

PlotRangeTuner::ValueType PlotRangeTuner::FindMinLimitX(const Hist& h, bool /*consired_errors*/)
{
    for(Int_t i = 1; i <= h.GetNbinsX(); ++i) {
        if(h.GetBinContent(i) != ValueType(0))
            return h.GetBinLowEdge(i);
    }
    return std::numeric_limits<ValueType>::max();
}

PlotRangeTuner::ValueType PlotRangeTuner::FindMaxLimitX(const Hist& h, bool /*consired_errors*/)
{
    for(Int_t i = h.GetNbinsX(); i > 0; --i) {
        if(h.GetBinContent(i) != ValueType(0))
            return h.GetBinLowEdge(i) + h.GetBinWidth(i);
    }
    return std::numeric_limits<ValueType>::lowest();
}

PlotRangeTuner::ValueType PlotRangeTuner::FindMinLimitY(const Hist& h, bool consired_errors)
{
    ValueType min = std::numeric_limits<ValueType>::max();
    for(int i = 1; i <= h.GetNbinsX(); ++i) {
        if(h.GetBinContent(i) != ValueType(0)) {
            ValueType v = h.GetBinContent(i);
            if(consired_errors)
                v -= h.GetBinErrorLow(i);
            min = std::min(min, v);
        }
    }
    return min;
}

PlotRangeTuner::ValueType PlotRangeTuner::FindMaxLimitY(const Hist& h, bool consired_errors)
{
    ValueType max = std::numeric_limits<ValueType>::lowest();
    for(int i = 1; i <= h.GetNbinsX(); ++i) {
        if(h.GetBinContent(i) != ValueType(0)) {
            ValueType v = h.GetBinContent(i);
            if(consired_errors)
                v += h.GetBinErrorUp(i);
            max = std::max(max, v);
        }
    }
    return max;
}

PlotRangeTuner::ValueType PlotRangeTuner::FindMinLimitX(const Graph& g, bool consired_errors)
{
    ValueType min = std::numeric_limits<ValueType>::max();
    for(int i = 0; i < g.GetN(); ++i) {
        ValueType v = g.GetX()[i];
        if(consired_errors)
            v -= g.GetEXlow()[i];
        min = std::min(min, v);
    }
    return min;
}

PlotRangeTuner::ValueType PlotRangeTuner::FindMaxLimitX(const Graph& g, bool consired_errors)
{
    ValueType max = std::numeric_limits<ValueType>::lowest();
    for(int i = 0; i < g.GetN(); ++i) {
        ValueType v = g.GetX()[i];
        if(consired_errors)
            v += g.GetEXhigh()[i];
        max = std::max(max, v);
    }
    return max;
}

PlotRangeTuner::ValueType PlotRangeTuner::FindMinLimitY(const Graph& g, bool consired_errors)
{
    ValueType min = std::numeric_limits<ValueType>::max();
    for(int i = 0; i < g.GetN(); ++i) {
        ValueType v = g.GetY()[i];
        if(consired_errors)
            v -= g.GetEYlow()[i];
        min = std::min(min, v);
    }
    return min;
}

PlotRangeTuner::ValueType PlotRangeTuner::FindMaxLimitY(const Graph& g, bool consired_errors)
{
    ValueType max = std::numeric_limits<ValueType>::lowest();
    for(int i = 0; i < g.GetN(); ++i) {
        ValueType v = g.GetY()[i];
        if(consired_errors)
            v += g.GetEYhigh()[i];
        max = std::max(max, v);
    }
    return max;
}

PlotRangeTuner::ValueType PlotRangeTuner::GetYMinValue(bool log_y, ValueType min_y_sf, ValueType y_min_log) const
{
    const ValueType y_min_value = y_min == std::numeric_limits<ValueType>::max()
            || y_min == -std::numeric_limits<ValueType>::infinity() ? 0 : y_min;
    return log_y ? std::max(y_min_value * min_y_sf, y_min_log) : y_min_value * min_y_sf;
}

PlotRangeTuner::ValueType PlotRangeTuner::GetYMaxValue(ValueType max_y_sf, ValueType y_min_value) const
{
    const ValueType y_max_value = y_max * max_y_sf;
    return y_max_value <= y_min_value || y_max_value == std::numeric_limits<ValueType>::infinity()
            ? y_min_value + 1 : y_max_value;
}

PlotRangeTuner::ValueType PlotRangeTuner::GetXMinValue() const
{
    return x_min == std::numeric_limits<ValueType>::max() || x_min == -std::numeric_limits<ValueType>::infinity()
            ? 0 : x_min;
}

PlotRangeTuner::ValueType PlotRangeTuner::GetXMaxValue() const
{
    return x_max == std::numeric_limits<ValueType>::lowest() || x_max == GetXMinValue()
            || x_max == std::numeric_limits<ValueType>::infinity() ? GetXMinValue() + 1 : x_max;
}

TH1F* PlotRangeTuner::DrawFrame(TPad& pad, bool log_y, ValueType max_y_sf, ValueType min_y_sf,
                                ValueType y_min_log) const
{
    const ValueType y_min_value = GetYMinValue(log_y, min_y_sf, y_min_log);
    return pad.DrawFrame(GetXMinValue(), y_min_value, GetXMaxValue(), GetYMaxValue(max_y_sf, y_min_value));
}

void PlotRangeTuner::SetRangeX(TAxis& x_axis) const
{
    x_axis.SetRangeUser(GetXMinValue(), GetXMaxValue());
}

void PlotRangeTuner::SetRangeY(TAxis& y_axis, bool log_y, ValueType max_y_sf, ValueType min_y_sf,
                               ValueType y_min_log) const
{
    const ValueType y_min_value = GetYMinValue(log_y, min_y_sf, y_min_log);
    y_axis.SetRangeUser(y_min_value, GetYMaxValue(max_y_sf, y_min_value));
}

void PlotRangeTuner::UpdateLimitsX(const Hist& h)
{
    const ValueType h_x_min = h.GetBinLowEdge(1);
    const ValueType h_x_max = h.GetBinLowEdge(h.GetNbinsX()) + h.GetBinWidth(h.GetNbinsX());
    x_min = std::min(x_min, h_x_min);
    x_max = std::max(x_max, h_x_max);
}

void PlotRangeTuner::UpdateLimitsX(const Graph& g)
{
    const ValueType g_x_min = FindMinLimitX(g, true);
    const ValueType g_x_max = FindMaxLimitX(g, true);
    x_min = std::min(x_min, g_x_min);
    x_max = std::max(x_max, g_x_max);
}


namespace plotting {

void DivideByBinWidth(TH1& histogram)
{
    for(Int_t n = 1; n <= histogram.GetNbinsX(); ++n) {
        const double new_value = histogram.GetBinContent(n) / histogram.GetBinWidth(n);
        const double new_bin_error = histogram.GetBinError(n) / histogram.GetBinWidth(n);
        histogram.SetBinContent(n, new_value);
        histogram.SetBinError(n, new_bin_error);
    }
}

void ApplyAdditionalUncertainty(TH1& histogram, double unc)
{
    for(Int_t n = 0; n <= histogram.GetNbinsX() + 1; ++n) {
        const double bin_value = histogram.GetBinContent(n);
        const double bin_error = histogram.GetBinError(n);
        const double new_error = std::hypot(bin_error, bin_value * unc);
        histogram.SetBinError(n, new_error);
    }
}

} // namespace plotting
} // root_ext
