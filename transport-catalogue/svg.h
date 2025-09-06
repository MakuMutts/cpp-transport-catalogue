#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace svg {

    struct Point {
        Point() = default;
        Point(double x, double y)
            : x(x), y(y) {
        }
        double x = 0;
        double y = 0;
    };

    struct Rgb {
        Rgb(unsigned r, unsigned g, unsigned b) {
            red = static_cast<uint8_t>(r);
            green = static_cast<uint8_t>(g);
            blue = static_cast<uint8_t>(b);
        }
        Rgb() = default;
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    struct Rgba {
        Rgba(unsigned r, unsigned g, unsigned b, double opac) {
            red = static_cast<uint8_t>(r);
            green = static_cast<uint8_t>(g);
            blue = static_cast<uint8_t>(b);
            opacity = opac;
        }
        Rgba() = default;
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        double opacity = 1.0;
    };

    struct OstreamColorPrinter {
        std::ostream& out;
        void operator()(std::monostate) const;
        void operator()(std::string color) const;
        void operator()(Rgb color) const;
        void operator()(Rgba color) const;
    };

    using Color = std::variant<std::monostate, Rgb, Rgba, std::string>;

    inline std::ostream& operator<<(std::ostream& out, Color color) {
        std::visit(OstreamColorPrinter{ out }, color);
        return out;
    }
    inline const Color NoneColor{};

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };
    inline std::ostream& operator<<(std::ostream& os, const StrokeLineCap& stroke) {
        using namespace std::literals;

        if (stroke == StrokeLineCap::BUTT) {
            os << "butt"sv;
        }
        if (stroke == StrokeLineCap::ROUND) {
            os << "round"sv;
        }
        if (stroke == StrokeLineCap::SQUARE) {
            os << "square"sv;
        }
        return os;
    }
    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };
    inline std::ostream& operator<<(std::ostream& os, const StrokeLineJoin& line) {
        using namespace std::literals;

        if (line == StrokeLineJoin::ARCS) {
            os << "arcs"sv;
        }
        if (line == StrokeLineJoin::BEVEL) {
            os << "bevel"sv;
        }
        if (line == StrokeLineJoin::MITER) {
            os << "miter"sv;
        }
        if (line == StrokeLineJoin::MITER_CLIP) {
            os << "miter-clip"sv;
        }
        if (line == StrokeLineJoin::ROUND) {
            os << "round"sv;
        }
        return os;
    }
    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out), indent_step(indent_step), indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeWidth(double width) {
            stroke_width_ = std::move(width);
            return AsOwner();
        }
        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            stroke_linecap_ = std::move(line_cap);
            return AsOwner();
        }
        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            stroke_linejoin_ = line_join;
            return AsOwner();
        }

    protected:
        ~PathProps() = default;

        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (fill_color_) {
                out << " fill=\""sv << *fill_color_ << "\""sv;
            }

            if (stroke_color_) {
                out << " stroke=\""sv << *stroke_color_ << "\""sv;
            }

            if (stroke_width_) {
                out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
            }
            if (stroke_linecap_) {
                out << " stroke-linecap=\""sv << *stroke_linecap_ << "\""sv;
            }
            if (stroke_linejoin_) {
                out << " stroke-linejoin=\""sv << *stroke_linejoin_ << "\""sv;
            }
        }

    private:
        Owner& AsOwner() {
            return static_cast<Owner&>(*this);
        }

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> stroke_linecap_;
        std::optional<StrokeLineJoin> stroke_linejoin_;
    };

    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    class ObjectContainer {
    public:
        template <typename Obj>
        void Add(Obj obj) {
            AddPtr(std::make_unique<Obj>(obj));
        }
        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
    };

    class Drawable {
    public:
        virtual void Draw(ObjectContainer& g) const = 0;
        virtual ~Drawable() = default;
    };

    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_ = { 0.0, 0.0 };
        double radius_ = 1.0;
    };

    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        // Добавляет очередную вершину к ломаной линии
        Polyline& AddPoint(Point point);
    private:
        void RenderObject(const RenderContext& context) const override;

        std::vector<Point> points_;
    };

    class Text final : public Object, public PathProps<Text> {
    public:
        // Задаёт координаты опорной точки (атрибуты x и y)
        Text& SetPosition(Point pos);

        // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        Text& SetOffset(Point offset);

        // Задаёт размеры шрифта (атрибут font-size)
        Text& SetFontSize(uint32_t size);

        // Задаёт название шрифта (атрибут font-family)
        Text& SetFontFamily(std::string font_family);

        // Задаёт толщину шрифта (атрибут font-weight)
        Text& SetFontWeight(std::string font_weight);

        // Задаёт текстовое содержимое объекта (отображается внутри тега text)
        Text& SetData(std::string data);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point position_ = { 0.0, 0.0 };
        Point offset_ = { 0.0, 0.0 };
        size_t font_size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_;
    };

    class Document : public ObjectContainer {
    public:
        void AddPtr(std::unique_ptr<Object>&& obj) {
            objects_ptr_.emplace_back(std::move(obj));
        }

        // Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const;

    private:
        std::vector<std::unique_ptr<Object>> objects_ptr_;
    };
    Polyline CreateStar(Point center, double outer_rad, double inner_rad, int num_rays);

}  // namespace svg