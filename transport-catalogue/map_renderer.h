#pragma once

#include "svg.h"
#include "geo.h"
#include "json.h"
#include "transport_catalogue.h"

#include <sstream>
#include <algorithm>

namespace renderer {

	inline const double EPSILON = 1e-6;
	bool IsZero(double value);

	class SphereProjector {
	public:

		template <typename PointInputIt>
		SphereProjector(PointInputIt points_begin, PointInputIt points_end,
			double max_width, double max_height, double padding)
			: padding_(padding)
		{
			if (points_begin == points_end) {
				return;
			}

			auto [left_it, right_it] = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
			min_lon_ = left_it->lng;
			double max_lon = right_it->lng;

			auto [bottom_it, top_it] = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
			double min_lat = bottom_it->lat;
			max_lat_ = top_it->lat;

			std::optional<double> width_approx;
			if (!IsZero(max_lon - min_lon_)) {
				width_approx = (max_width - 2 * padding) / (max_lon - min_lon_);
			}

			std::optional<double> height_approx;
			if (!IsZero(max_lat_ - min_lat)) {
				height_approx = (max_height - 2 * padding) / (max_lat_ - min_lat);
			}

			if (width_approx && height_approx) {
				approx_ = std::min(*width_approx, *height_approx);
			}
			else if (width_approx) {
				approx_ = *width_approx;
			}
			else if (height_approx) {
				approx_ = *height_approx;
			}
		}

		svg::Point operator()(geo::Coordinates coordinates) const {
			const double x = (coordinates.lng - min_lon_) * approx_ + padding_;
			const double y = (max_lat_ - coordinates.lat) * approx_ + padding_;
			return { x, y };
		}

	private:
		double padding_;
		double min_lon_ = 0;
		double max_lat_ = 0;
		double approx_ = 0;
	};

	struct RenderSettings {
		double width = 0.0;
		double height = 0.0;
		double padding = 0.0;
		double stop_radius = 0.0;
		double line_width = 0.0;
		int bus_label_font_size = 0;
		svg::Point bus_label_offset = { 0.0, 0.0 };
		int stop_label_font_size = 0;
		svg::Point stop_label_offset = { 0.0, 0.0 };
		svg::Color underlayer_color = { svg::NoneColor };
		double underlayer_width = 0.0;
		std::vector<svg::Color> color_palette{};
	};


	class MapRenderer {
	public:
		MapRenderer(const RenderSettings& render_settings)
			: render_settings_(render_settings)
		{
		}

		svg::Document RendererSVG(const std::map<std::string_view, const transport::Bus*>& buses) const;

		const json::Node RendererPrintMap(const transport::Catalog& catalog, const json::Dict& request_map) const;

	private:
		const RenderSettings render_settings_;

		std::vector<svg::Polyline> RendererRouteLines(const std::map<std::string_view, const transport::Bus*>& buses, const SphereProjector& sprojector) const;
		std::vector<svg::Text> RendererBusLabel(const std::map<std::string_view, const transport::Bus*>& buses, const SphereProjector& sprojector) const;
		std::vector<svg::Circle> RendererStopsSymbols(const std::map<std::string_view, const transport::Stop*>& stops, const SphereProjector& sprojector) const;
		std::vector<svg::Text> RendererStopsLabels(const std::map<std::string_view, const transport::Stop*>& stops, const SphereProjector& sprojector) const;

	};

}