/*! Definition of the plot primitives.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <sstream>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <TROOT.h>
#include <TColor.h>
#include <TAttText.h>
#include "EnumNameMap.h"
#include "NumericPrimitives.h"

namespace root_ext {

template<typename T, size_t n_dim, bool positively_defined = false>
struct Point;

template<typename T, bool positively_defined>
struct Point<T, 1, positively_defined> {
    Point() : _x(0) {}
    Point(const T& x)
        : _x(x)
    {
        if(!IsValid(x))
            throw analysis::exception("Invalid point %1%.") % x;
    }

    const T& x() const { return _x; }
    operator T() const { return _x; }
    Point<T, 1, positively_defined> operator+(const Point<T, 1, positively_defined>& other) const
    {
        return Point<T, 1, positively_defined>(x() + other.x());
    }
    Point<T, 1, positively_defined> operator-(const Point<T, 1, positively_defined>& other) const
    {
        return Point<T, 1, positively_defined>(x() - other.x());
    }
    Point<T, 1, positively_defined> operator*(const Point<T, 1, positively_defined>& other) const
    {
        return Point<T, 1, positively_defined>(x() * other.x());
    }

    static bool IsValid(const T& x) { return !positively_defined || x >= 0; }

private:
    T _x;
};

template<typename T, bool positively_defined>
struct Point<T, 2, positively_defined> {
    Point() : _x(0), _y(0) {}
    Point(const T& x, const T& y)
        : _x(x), _y(y)
    {
        if(!IsValid(x, y))
            throw analysis::exception("Invalid point (%1%, %2%).") % x % y;
    }

    const T& x() const { return _x; }
    const T& y() const { return _y; }
    Point<T, 2, positively_defined> operator+(const Point<T, 2, positively_defined>& other) const
    {
        return Point<T, 2, positively_defined>(x() + other.x(), y() + other.y());
    }
    Point<T, 2, positively_defined> operator-(const Point<T, 2, positively_defined>& other) const
    {
        return Point<T, 2, positively_defined>(x() - other.x(), y() - other.y());
    }
    Point<T, 2, positively_defined> operator*(const Point<T, 2, positively_defined>& other) const
    {
        return Point<T, 2, positively_defined>(x() * other.x(), y() * other.y());
    }

    Point<T, 2, positively_defined> flip_x() const { return Point<T, 2, positively_defined>(-x(), y()); }
    Point<T, 2, positively_defined> flip_y() const { return Point<T, 2, positively_defined>(x(), -y()); }

    static bool IsValid(const T& x, const T& y) { return !positively_defined || (x >= 0 && y >= 0); }

private:
    T _x, _y;
};

template<typename T, bool positively_defined>
std::ostream& operator<<(std::ostream& s, const Point<T, 2, positively_defined>& p)
{
    s << boost::format("%1% %2%") % p.x() % p.y();
    return s;
}

template<typename T, bool positively_defined>
std::istream& operator>>(std::istream& s, Point<T, 2, positively_defined>& p)
{
    T x, y;
    s >> x >> y;
    if(s.fail())
        throw analysis::exception("Invalid point.");
    p = Point<T, 2, positively_defined>(x, y);
    return s;
}

template<typename T, bool positively_defined>
std::ostream& operator<<(std::ostream& s, const Point<T, 1, positively_defined>& p)
{
    s << p.x();
    return s;
}

template<typename T, bool positively_defined>
std::istream& operator>>(std::istream& s, Point<T, 1, positively_defined>& p)
{
    T x;
    s >> x;
    if(s.fail())
        throw analysis::exception("Invalid point.");
    p = Point<T, 1, positively_defined>(x);
    return s;
}


template<typename T>
struct Box {
    using Position = Point<T, 2>;
    Box() {}

    Box(const Position& left_bottom, const Position& right_top) : _left_bottom(left_bottom), _right_top(right_top)
    {
        CheckValidity();
    }

    Box(const T& left_bottom_x, const T& left_bottom_y, const T& right_top_x, const T& right_top_y)
        : _left_bottom(left_bottom_x, left_bottom_y), _right_top(right_top_x, right_top_y)
    {
        CheckValidity();
    }

    const Position& left_bottom() const { return _left_bottom; }
    const Position& right_top() const { return _right_top; }
    const T& left_bottom_x() const { return left_bottom().x(); }
    const T& left_bottom_y() const { return left_bottom().y(); }
    const T& right_top_x() const { return right_top().x(); }
    const T& right_top_y() const { return right_top().y(); }

    static bool IsValid(const Position& left_bottom, const Position& right_top)
    {
        return IsValid(left_bottom.x(), left_bottom.y(), right_top.x(), right_top.y());
    }

    static bool IsValid(const T& left_bottom_x, const T& left_bottom_y, const T& right_top_x, const T& right_top_y)
    {
        return analysis::Range<T>::IsValid(left_bottom_x, right_top_x)
                && analysis::Range<T>::IsValid(left_bottom_y, right_top_y);
    }

private:
    void CheckValidity() const
    {
        if(!IsValid(_left_bottom, _right_top))
            throw analysis::exception("Invalid box [%1% %2%]") % _left_bottom % _right_top;
    }

private:
    Position _left_bottom, _right_top;
};

template<typename T>
std::ostream& operator<<(std::ostream& s, const Box<T>& b)
{
    s << b.left_bottom() << " " << b.right_top();
    return s;
}

template<typename T>
std::istream& operator>>(std::istream& s, Box<T>& b)
{
    typename Box<T>::Position left_bottom, right_top;
    try {
        s >> left_bottom >> right_top;
    }catch(analysis::exception&) {
        throw analysis::exception("Invalid box.");
    }
    b = Box<T>(left_bottom, right_top);
    return s;
}

template<typename T>
struct MarginBox {
    MarginBox() : _left(0), _bottom(0), _right(0), _top(0) {}

    MarginBox(const T& left, const T& bottom, const T& right, const T& top)
        : _left(left), _bottom(bottom), _right(right), _top(top)
    {
        CheckValidity();
    }

    const T& left() const { return _left; }
    const T& bottom() const { return _bottom; }
    const T& right() const { return _right; }
    const T& top() const { return _top; }

    static bool IsValid(const T& left, const T& bottom, const T& right, const T& top)
    {
        return IsValidMarginValue(left) && IsValidMarginValue(bottom)
                && IsValidMarginValue(right) && IsValidMarginValue(top);
    }

    static bool IsValidMarginValue(const T& v) { return v >= 0 && v <= 1; }

private:
    void CheckValidity() const
    {
        if(!IsValid(_left, _bottom, _right, _top))
            throw analysis::exception("Invalid margin box: (left, bottom, right, top) = (%1%, %2%, %3%, %4%).")
                % _left % _bottom % _right % _top;
    }

private:
    T _left, _bottom, _right, _top;
};

template<typename T>
std::ostream& operator<<(std::ostream& s, const MarginBox<T>& b)
{
    s << boost::format("%1% %2% %3% %4%") % b.left() % b.bottom() % b.right() % b.top();
    return s;
}

template<typename T>
std::istream& operator>>(std::istream& s, MarginBox<T>& b)
{
    T left, bottom, right, top;
    s >> left >> bottom >> right >> top;
    if(s.fail())
        throw analysis::exception("Invalid margin box.");
    b = MarginBox<T>(left, bottom, right, top);
    return s;
}

namespace detail {

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

} // namespace detail

class Color {
public:
    Color() { RetrieveTColor(kBlack); }
    explicit Color(int color_id) { RetrieveTColor(color_id); }
    explicit Color(const std::string& hex_color)
    {
        static const boost::regex hex_color_pattern("^#(?:[0-9a-fA-F]{2}){3}$");
        if(!boost::regex_match(hex_color, hex_color_pattern))
            throw analysis::exception("Invalid color '%1%'.") % hex_color;
        const int color_id = TColor::GetColor(hex_color.c_str());
        RetrieveTColor(color_id);
    }

    bool IsSimple() const { return detail::ReferenceColorCollection::IsReferenceColor(GetColorId()); }
    int GetColorId() const { return GetTColor().GetNumber(); }
    const TColor& GetTColor() const { return *t_color; }

    std::string ToString() const
    {
        if(IsSimple())
            return detail::ReferenceColorCollection::ToString(GetColorId());
        return GetTColor().AsHexString();
    }

private:
    static void CheckComponent(const std::string& name, int value)
    {
        static const analysis::Range<int> color_range(0, 255);
        if(!color_range.Contains(value))
            throw analysis::exception("Invalid color %1% component value = %2%") % name % value;
    }

    void RetrieveTColor(int color_id)
    {
        t_color = gROOT->GetColor(color_id);
        if(!t_color)
            throw analysis::exception("Color with id %1% is not defined.") % color_id;
    }

private:
    const TColor* t_color;
};

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
    if(detail::ReferenceColorCollection::TryParse(name, e_color, shift)) {
        c = Color(detail::ReferenceColorCollection::ToColorId(e_color, shift));
    } else
        c = Color(name);
    return s;
}

class Font {
public:
    using Integer = short;

    Font() : _number(4), _precision(2) {}
    explicit Font(Integer font_code)
    {
        ParseFontCode(font_code, _number, _precision);
        CheckValidity();
    }
    Font(Integer font_number, Integer precision)
        : _number(font_number), _precision(precision)
    {
        CheckValidity();
    }

    Integer code() const { return MakeFontCode(_number, _precision); }
    Integer number() const { return _number; }
    Integer precision() const { return _precision; }

    static Integer MakeFontCode(Integer font_number, Integer precision) { return font_number * 10 + precision; }
    static void ParseFontCode(Integer font_code, Integer& font_number, Integer& precision)
    {
        font_number = font_code / 10;
        precision = font_code % 10;
    }

    static bool IsValid(Integer font_number, Integer precision)
    {
        static const analysis::Range<Integer> font_number_range(1, 15);
        static const analysis::Range<Integer> precision_range(0, 3);
        return font_number_range.Contains(font_number) && precision_range.Contains(precision);
    }

    static bool IsValid(Integer font_code)
    {
        Integer font_number, precision;
        ParseFontCode(font_code, font_number, precision);
        return IsValid(font_number, precision);
    }

private:
    void CheckValidity() const
    {
        if(!IsValid(number(), precision()))
            throw analysis::exception("Invalid font code = %1%.") % code();
    }

private:
    Integer _number, _precision;
};

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

enum class TextAlign { LeftBottom = kHAlignLeft + kVAlignBottom, LeftCenter = kHAlignLeft + kVAlignCenter,
                       LeftTop = kHAlignLeft + kVAlignTop, CenterBottom = kHAlignCenter + kVAlignBottom,
                       Center = kHAlignCenter + kVAlignCenter, CenterTop = kHAlignCenter + kVAlignTop,
                       RightBottom = kHAlignRight + kVAlignBottom, RightCenter = kHAlignRight + kVAlignCenter,
                       RightTop = kHAlignRight + kVAlignTop };
ENUM_NAMES(TextAlign) = {
    { TextAlign::LeftBottom, "left_bottom" },
    { TextAlign::LeftCenter, "left_center" },
    { TextAlign::LeftTop, "left_top" },
    { TextAlign::CenterBottom, "center_bottom" },
    { TextAlign::Center, "center" },
    { TextAlign::CenterTop, "center_top" },
    { TextAlign::RightBottom, "right_bottom" },
    { TextAlign::RightCenter, "right_center" },
    { TextAlign::RightTop, "right_top" }
};

} // namespace root_ext
