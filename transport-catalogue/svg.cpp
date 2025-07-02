#define _USE_MATH_DEFINES
#include "svg.h"

#include <cmath>

namespace svg {

    using namespace std::literals;

    void OstreamColorPrinter::operator()(std::monostate) const {
        out << "none"sv;
    }

    void OstreamColorPrinter::operator()(std::string color) const {
        out << color;
    }

    void OstreamColorPrinter::operator()(Rgb color) const {
        out << "rgb("sv << int(color.red) << ","sv << int(color.green)
            << ","sv << int(color.blue) << ")"sv;
    }

    void OstreamColorPrinter::operator()(Rgba color) const {
        out << "rgba("sv << int(color.red) << ","sv << int(color.green)
            << ","sv << int(color.blue) << ","sv << color.opacity << ")"sv;
    }

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        // Выводим атрибуты, унаследованные от PathProps
        RenderAttrs(context.out);
        out << "/>"sv;
    }

    // ---------- Polyline ------------------

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv;
        std::string delimiter = "";
        for (const Point point : points_) {
            out << delimiter << point.x << "," << point.y;
            delimiter = " "sv;
        }
        out << "\""sv;
        RenderAttrs(context.out);
        out << "/>"sv;
    }

    // ---------- Text ------------------

    Text& Text::SetPosition(Point pos) {
        position_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = font_family;
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = font_weight;
        return *this;
    }

    Text& Text::SetData(std::string data) {
        std::string result;
        for (char c : data) {
            if (c == '"') {
                result += "&quot;";
                continue;
            }
            if (c == '\'') {
                result += "&apos;";
                continue;
            }
            if (c == '<') {
                result += "&lt;";
                continue;
            }
            if (c == '>') {
                result += "&gt;";
                continue;
            }
            if (c == '&') {
                result += "&amp;";
                continue;
            }
            result += c;
        }
        data_ = result;
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text"sv;
        RenderAttrs(context.out);
        out << " x=\""sv << position_.x << "\" y=\""sv << position_.y << "\" "sv;
        out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
        out << "font-size=\""sv << font_size_ << "\""sv;
        if (!font_family_.empty()) {
            out << " font-family=\""sv << font_family_ << "\""sv;
        }
        if (!font_weight_.empty()) {
            out << " font-weight=\""sv << font_weight_ << "\""sv;
        }

        out << ">"sv;
        out << data_ << "</text>"sv;
    }

    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
        RenderContext ctx(out, 2, 2);
        for (const auto& obj : objects_ptr_) {
            obj.get()->Render(ctx);
        }

        out << "</svg>"sv;
    }

    Polyline CreateStar(Point center, double outer_rad, double inner_rad, int num_rays) {
        Polyline polyline;
        for (int i = 0; i <= num_rays; ++i) {
            double angle = 2 * M_PI * (i % num_rays) / num_rays;
            polyline.AddPoint({ center.x + outer_rad * sin(angle), center.y - outer_rad * cos(angle) }).SetFillColor("red").SetStrokeColor("black");
            if (i == num_rays) {
                break;
            }
            angle += M_PI / num_rays;
            polyline.AddPoint({ center.x + inner_rad * sin(angle), center.y - inner_rad * cos(angle) }).SetFillColor("red").SetStrokeColor("black");
        }
        return polyline;
    }
}  // namespace svg
