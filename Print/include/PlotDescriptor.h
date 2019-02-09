/*! Base class for various plot descriptors.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <memory>
#include <vector>

class TPad;
class TLegend;
class TObject;

namespace root_ext {

struct PlotDescriptor {
    virtual ~PlotDescriptor() {}
    virtual bool HasPrintableContent() const = 0;
    virtual void Draw(std::shared_ptr<TPad> main_pad, std::shared_ptr<TPad> ratio_pad, std::shared_ptr<TLegend> legend,
                      std::vector<std::shared_ptr<TObject>>& plot_items) = 0;
};

}
