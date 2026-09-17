#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <spdlog/sinks/basic_file_sink.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

// windows.h (pulled in by spdlog) defines GetObject, which mangles BSScript::Variable::GetObject.
#undef GetObject

using namespace std::literals;

namespace logs = SKSE::log;
