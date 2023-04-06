#pragma once

#include <stuff/core.hpp>

#include <imgui.h>

namespace trc {

template<typename T>
struct imgui_type;

#pragma push_macro("IMGUI_TYPE_FACTORY")

#define IMGUI_TYPE_FACTORY(_std_type, _imgui_type) \
template<> \
struct imgui_type<_std_type> : std::integral_constant<ImGuiDataType, ImGuiDataType_##_imgui_type> {};

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
static constexpr ImGuiDataType imgui_type_v = imgui_type<T>::value;

}
