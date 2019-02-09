/*! Definition of the plot primitives.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include "AnalysisTools/Print/include/PlotPrimitives.h"

#include <boost/regex.hpp>
#include <TColor.h>
#include <TROOT.h>

namespace root_ext {

namespace {

class ReferenceColorCollection {
public:
    using ColorNameMap = analysis::EnumNameMap<EColor>;
    using ColorRelativeRangeMap = std::unordered_map<EColor, analysis::RelativeRange<int>, ColorNameMap::EnumHash>;
    using ColorRangeMap = std::unordered_map<EColor, analysis::Range<int>, ColorNameMap::EnumHash>;

    static const ColorNameMap& GetReferenceColorNames()
    {
        static const ColorNameMap names = ColorNameMap("EColor", {
            { kWhite, "kWhite" }, { kBlack, "kBlack" }, { kGray, "kGray" }, { kRed, "kRed" }, { kGreen, "kGreen" },
            { kBlue, "kBlue" }, { kYellow, "kYellow" }, { kMagenta, "kMagenta" }, { kCyan, "kCyan" },
            { kOrange, "kOrange" }, { kSpring, "kSpring" }, { kTeal, "kTeal" }, { kAzure, "kAzure" },
            { kViolet, "kViolet" }, { kPink, "kPink" }
        });
        return names;
    }

    static const ColorRelativeRangeMap& GetReferenceColorRelativeRanges()
    {
        static const ColorRelativeRangeMap ranges = {
            { kWhite, { 0, 0 } }, { kBlack, { 0, 0 } }, { kGray, { 0, 3 } }, { kRed, { -10, 4 } },
            { kGreen, { -10, 4 } }, { kBlue, { -10, 4 } }, { kYellow, { -10, 4 } }, { kMagenta, { -10, 4 } },
            { kCyan, { -10, 4 } }, { kOrange, { -9, 10 } }, { kSpring, { -9, 10 } }, { kTeal, { -9, 10 } },
            { kAzure, { -9, 10 } }, { kViolet, { -9, 10 } }, { kPink, { -9, 10 } }
        };
        return ranges;
    }

    static const ColorRangeMap& GetReferenceColorRanges()
    {
        static ColorRangeMap ranges;
        if(!ranges.size()) {
            for(const auto& rel_range : GetReferenceColorRelativeRanges())
                ranges[rel_range.first] = rel_range.second.ToAbsoluteRange(static_cast<int>(rel_range.first));
        }
        return ranges;
    }

    static bool FindReferenceColor(int color_id, EColor& e_color, int& shift)
    {
        for(const auto& range : GetReferenceColorRanges()) {
            if(range.second.Contains(color_id)) {
                e_color = range.first;
                shift = color_id - static_cast<int>(range.first);
                return true;
            }
        }
        return false;
    }

    static bool IsReferenceColor(int color_id)
    {
        EColor e_color;
        int shift;
        return FindReferenceColor(color_id, e_color, shift);
    }

    static int ToColorId(EColor e_color, int shift)
    {
        return static_cast<int>(e_color) + shift;
    }

    static std::string ToString(EColor e_color, int shift)
    {
        std::ostringstream ss;
        ss << GetReferenceColorNames().EnumToString(e_color);
        if(shift > 0)
            ss << "+" << shift;
        else if(shift < 0)
            ss << shift;
        return ss.str();
    }

    static std::string ToString(int color_id)
    {
        EColor e_color;
        int shift;
        if(!FindReferenceColor(color_id, e_color, shift))
            throw analysis::exception("Unable to find a reference color for the color with id = %1%.") % color_id;
        return ToString(e_color, shift);
    }

    static bool TryParse(const std::string& str, EColor& e_color, int& shift)
    {
        if(GetReferenceColorNames().TryParse(str, e_color)) {
            shift = 0;
            return true;
        }
        std::vector<std::string> sub_str;
        boost::split(sub_str, str, boost::is_any_of("+-"), boost::token_compress_on);
        if(sub_str.size() != 2 || !GetReferenceColorNames().TryParse(sub_str.at(0), e_color))
            return false;
        std::istringstream ss(sub_str.at(1));
        ss >> shift;
        int sign = std::find(str.begin(), str.end(), '-') == str.end() ? 1 : -1;
        shift *= sign;
        return !ss.fail();
    }
};

} // anonymous namespace

Color::Color() { RetrieveTColor(kBlack); }
Color::Color(int color_id) { RetrieveTColor(color_id); }
Color::Color(const std::string& hex_color)
{
    static const boost::regex hex_color_pattern("^#(?:[0-9a-fA-F]{2}){3}$");
    if(!boost::regex_match(hex_color, hex_color_pattern))
        throw analysis::exception("Invalid color '%1%'.") % hex_color;
    const int color_id = TColor::GetColor(hex_color.c_str());
    RetrieveTColor(color_id);
}

bool Color::IsSimple() const { return ReferenceColorCollection::IsReferenceColor(GetColorId()); }
int Color::GetColorId() const { return GetTColor().GetNumber(); }
Color_t Color::GetColor_t() const { return static_cast<Color_t>(GetColorId()); }
const TColor& Color::GetTColor() const { return *t_color; }

std::string Color::ToString() const
{
    if(IsSimple())
        return ReferenceColorCollection::ToString(GetColorId());
    return GetTColor().AsHexString();
}

Color Color::CreateTransparentCopy(float alpha) const
{
    return Color(TColor::GetColorTransparent(GetColorId(), alpha));
}

void Color::CheckComponent(const std::string& name, int value)
{
    static const analysis::Range<int> color_range(0, 255);
    if(!color_range.Contains(value))
        throw analysis::exception("Invalid color %1% component value = %2%") % name % value;
}

void Color::RetrieveTColor(int color_id)
{
    t_color = gROOT->GetColor(color_id);
    if(!t_color)
        throw analysis::exception("Color with id %1% is not defined.") % color_id;
}

std::ostream& operator<<(std::ostream& s, const Color& c)
{
    s << c.ToString();
    return s;
}

std::istream& operator>>(std::istream& s, Color& c)
{
    std::string name;
    s >> name;
    EColor e_color;
    int shift;
    if(ReferenceColorCollection::TryParse(name, e_color, shift)) {
        c = Color(ReferenceColorCollection::ToColorId(e_color, shift));
    } else
        c = Color(name);
    return s;
}

Font::Font() : _number(4), _precision(2) {}

Font::Font(Integer font_code)
{
    ParseFontCode(font_code, _number, _precision);
    CheckValidity();
}

Font::Font(Integer font_number, Integer precision)
    : _number(font_number), _precision(precision)
{
    CheckValidity();
}

Font::Integer Font::code() const { return MakeFontCode(_number, _precision); }
Font::Integer Font::number() const { return _number; }
Font::Integer Font::precision() const { return _precision; }

Font::Integer Font::MakeFontCode(Integer font_number, Integer precision) { return font_number * 10 + precision; }

void Font::ParseFontCode(Integer font_code, Integer& font_number, Integer& precision)
{
    font_number = font_code / 10;
    precision = font_code % 10;
}

bool Font::IsValid(Integer font_number, Integer precision)
{
    static const analysis::Range<Integer> font_number_range(1, 15);
    static const analysis::Range<Integer> precision_range(0, 3);
    return font_number_range.Contains(font_number) && precision_range.Contains(precision);
}

bool Font::IsValid(Integer font_code)
{
    Integer font_number, precision;
    ParseFontCode(font_code, font_number, precision);
    return IsValid(font_number, precision);
}

void Font::CheckValidity() const
{
    if(!IsValid(number(), precision()))
        throw analysis::exception("Invalid font code = %1%.") % code();
}

std::ostream& operator<<(std::ostream& s, const Font& f)
{
    s << f.code();
    return s;
}

std::istream& operator>>(std::istream& s, Font& f)
{
    Font::Integer font_code;
    s >> font_code;
    f = Font(font_code);
    return s;
}

} // namespace root_ext
