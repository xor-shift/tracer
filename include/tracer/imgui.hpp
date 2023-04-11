#pragma once

#include <stuff/core.hpp>
#include <stuff/scope.hpp>

#include <tracer/common.hpp>

#include <imgui.h>

#include <optional>

namespace trc::imgui {

template<typename T>
struct data_type;

#pragma push_macro("IMGUI_TYPE_FACTORY")

#define IMGUI_TYPE_FACTORY(_std_type, _imgui_type) \
    template<>                                     \
    struct data_type<_std_type> : std::integral_constant<ImGuiDataType, ImGuiDataType_##_imgui_type> {};

IMGUI_TYPE_FACTORY(i8, S8);
IMGUI_TYPE_FACTORY(i16, S16);
IMGUI_TYPE_FACTORY(i32, S32);
IMGUI_TYPE_FACTORY(i64, S64);
IMGUI_TYPE_FACTORY(u8, U8);
IMGUI_TYPE_FACTORY(u16, U16);
IMGUI_TYPE_FACTORY(u32, U32);
IMGUI_TYPE_FACTORY(u64, U64);
IMGUI_TYPE_FACTORY(float, Float)
IMGUI_TYPE_FACTORY(double, Double)

#pragma pop_macro("IMGUI_TYPE_FACTORY")

template<typename T>
static constexpr ImGuiDataType data_type_v = data_type<T>::value;

template<typename T>
static auto input_scalar(const char* label, T& v, std::optional<T> step = std::nullopt, std::optional<T> step_fast = std::nullopt) -> bool {
    T* step_arg = step ? &*step : nullptr;
    T* step_fast_arg = step_fast ? &*step_fast : nullptr;

    return ImGui::InputScalar(label, data_type_v<T>, &v, step_arg, step_fast_arg);
}

template<typename T, usize Extent = std::dynamic_extent>
static auto input_scalar_n(const char* label, std::span<T, Extent> elems, std::optional<T> step = std::nullopt, std::optional<T> step_fast = std::nullopt) -> bool {
    T* step_arg = step ? &*step : nullptr;
    T* step_fast_arg = step_fast ? &*step_fast : nullptr;

    return ImGui::InputScalarN(label, data_type_v<T>, elems.data(), elems.size(), step_arg, step_fast_arg);
}

template<typename T>
static auto slider(const char* label, T& value, std::optional<T> min = std::nullopt, std::optional<T> max = std::nullopt, const char* format = nullptr) {
    T* min_arg = min ? &*min : nullptr;
    T* max_arg = max ? &*max : nullptr;

    return ImGui::SliderScalar(label, data_type_v<T>, &value, min_arg, max_arg, format);
}

template<std::floating_point T>
static auto slider_angle(const char* label, T& radians, T degrees_min = -360, T degrees_max = 360) {
    const char* format = nullptr;

    if constexpr (std::is_same_v<T, float>) {
        format = "%.3f deg";
    } else if constexpr (std::is_same_v<T, double>) {
        format = "%.3lf deg";
    }


    T degrees = radians * 360 * T(0.5) * std::numbers::inv_pi_v<T>;
    bool value_changed = slider<real>(label, degrees, degrees_min, degrees_max, format);
    radians = degrees * 2 * std::numbers::pi_v<T> / 360;

    return value_changed;
}

static auto guarded_group() {
    ImGui::BeginGroup();
    return stf::scope::scope_exit{[] { ImGui::EndGroup(); }};
}

static void help_marker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

}// namespace trc::imgui
