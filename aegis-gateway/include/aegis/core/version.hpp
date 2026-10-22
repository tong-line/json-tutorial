#pragma once

#include <string_view>

namespace aegis {

/// 返回网关版本号，与 CMake project(VERSION ...) 保持一致。
std::string_view version() noexcept;

}  // namespace aegis
