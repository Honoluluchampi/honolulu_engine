#pragma once

namespace hnll::game {

template <typename T>
concept Component = requires (T t) { t.get_component_id(); };

template <Component T>

} // namespace hnll::game