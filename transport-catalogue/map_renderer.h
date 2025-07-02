#pragma once
#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "domain.h"
#include "geo.h"
#include "svg.h"


namespace renderer {
    struct SvgRenderSettings
    {
        double width = 0.0;
        double height = 0.0;
        double padding = 0.0;
    };

    struct LabelRenderSetting {
        int font_size = 0;
        svg::Point offset = svg::Point(0,0);
    };

    struct BusRenderSettings
    {
        double line_width = 0.0;
        LabelRenderSetting label;
    };

    struct StopRenderSettings
    {
        double radius = 0.0;
        LabelRenderSetting label;
    };

    struct UnderlayerSettings {
        svg::Color color;
        double width = 0.0;
    };

    struct RenderSettings {
        SvgRenderSettings svg;
        BusRenderSettings bus;
        StopRenderSettings stop;
        UnderlayerSettings underlayer;
        std::vector<svg::Color> color_palette;
    };

    struct BusColor {
        const domain::Bus* bus;
        const svg::Color* color;
    };

    inline const double EPSILON = 1e-6;
    inline bool IsZero(double value) {
        return std::abs(value) < EPSILON;
    }
    //Этот клас наверное можно перенести в cpp
    class SphereProjector {
    public:
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
            double max_height, double padding)
            : padding_(padding) {
            if (points_begin == points_end) {
                return;
            }

            const auto [left_it, right_it] = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                return lhs.lng < rhs.lng;
                });
            min_lon_ = left_it->lng;
            const double max_lon = right_it->lng;

            const auto [bottom_it, top_it] = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                return lhs.lat < rhs.lat;
                });
            const double min_lat = bottom_it->lat;
            max_lat_ = top_it->lat;

            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom) {
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            }
            else if (width_zoom) {
                zoom_coeff_ = *width_zoom;
            }
            else if (height_zoom) {
                zoom_coeff_ = *height_zoom;
            }
        }

        svg::Point operator()(geo::Coordinates coords) const {
            return { (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                    (max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
        }

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    class MapRenderer {
    public:
        void SetRenderSettings(const RenderSettings& render_setings) {
            render_setings_ = render_setings;
        };

        std::vector<BusColor> GetBusLineColor(std::vector<const domain::Bus*>& buses) const;  //Получение цветов автобусов
        std::vector<svg::Polyline> GetRouteLines(const std::vector<BusColor>& sorted_by_name_buses_color,
            const SphereProjector& sphere_projector) const;  //Получение линий маршрутов
        std::vector<svg::Text> GetRouteNames(const std::vector<BusColor>& buses,
            const SphereProjector& sphere_projector) const;  //Получение названия маршрута
        std::vector<svg::Circle> GetStopSymbols(const std::map<std::string, const domain::Stop*>& stops,
            const SphereProjector& sphere_projector) const;  //Получение символов остановок
        std::vector<svg::Text> GetStopNames(const std::map<std::string, const domain::Stop*>& stops,
            const SphereProjector& sphere_projector) const;  //Получение названия остановок
        const RenderSettings& GetRenderSetings() const {
            return render_setings_;
        };
    private:
        
        RenderSettings render_setings_;
    };

}  // namespace renderer
