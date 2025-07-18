#ifndef OPENSIM_LOG_H_
#define OPENSIM_LOG_H_
/* -------------------------------------------------------------------------- *
 *                           OpenSim:  Logger.h                               *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2019 Stanford University and the Authors                *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "osimCommonDLL.h"              // for OSIMCOMMON_API
#include <memory>                       // for shared_ptr
#include <spdlog/common.h>              // for string_view_t
#include <spdlog/fmt/bundled/base.h>    // for formatter
#include <spdlog/fmt/bundled/ostream.h> // for ostream_formatter
#include <spdlog/logger.h>              // for logger
#include <string>                       // for basic_string, string

#ifndef SWIG
#include <SimTKcommon/SmallMatrix.h>             // for Vec3
#include <SimTKcommon/internal/BigMatrix.h>      // for Vector
#include <SimTKcommon/internal/MassProperties.h> // for Inertia
#include <SimTKcommon/internal/Rotation.h>       // for Rotation

// fmt library serializers for custom SimTK objects
template <> struct fmt::formatter<SimTK::Vec3> : ostream_formatter {};
template <> struct fmt::formatter<SimTK::Vector> : ostream_formatter {};
template <> struct fmt::formatter<SimTK::Rotation> : ostream_formatter {};
template <> struct fmt::formatter<SimTK::Inertia> : ostream_formatter {};
#endif

namespace OpenSim {

class LogSink;

/// This is a singleton class (single instance) for logging messages and
/// controlling how those messages are presented to the user.
class OSIMCOMMON_API Logger {
public:
    ///  This is a static singleton class: there is no way of constructing it
    Logger() = delete;

    /// This enum lists the types of messages that should be logged. These
    /// levels match those of the spdlog logging library that OpenSim uses for
    /// logging.
    enum class Level {
        /// Do not log any messages. Useful when running an optimization or
        /// automated pipeline.
        Off = 6,
        /// Only log critical errors.
        Critical = 5,
        /// Log all messages that require user intervention.
        Error = 4,
        /// Log warnings. Warnings are generated when the software will proceed
        /// but the user should check their input.
        Warn = 3,
        /// Default.
        Info = 2,
        /// Log information that may be useful when debugging the operation of
        /// the
        /// software to investigate unexpected results.
        Debug = 1,
        /// Log as much as possible, including messages that describe the
        /// software's
        /// behavior step by step. Note: OpenSim has very few Trace-level
        /// messages.
        Trace = 0
    };

    /// Log messages of importance `level` and greater.
    /// For example, if the level is set to Info, then Critical, Error, Warn,
    /// and Info messages are logged, while Debug and Trace messages are not
    /// logged.
    static void setLevel(Level level);
    static Level getLevel();

    /// Set the logging level using one of the following strings
    /// (case-insensitive):
    /// - Off
    /// - Critical
    /// - Error
    /// - Warn
    /// - Info
    /// - Debug
    /// - Trace
    /// This variant of setLevel() is for use in Matlab.
    /// @see Level.
    static void setLevelString(std::string level);
    static std::string getLevelString();

    /// Returns true if messages at the provided level should be logged,
    /// based on the set logging level. The following code will produce output:
    /// @code
    /// Log::setLevel(Log::Level::Warn);
    /// if (shouldLog(Log::Level::Error)) {
    ///     std::cout << "Error encountered." << std::endl;
    /// }
    /// @endcode
    static bool shouldLog(Level level);

    /// @name Commands to log messages
    /// Use these functions instead of using spdlog directly.
    /// @{

    template <typename... Args>
    static void critical(spdlog::string_view_t fmt, const Args&... args) {
        if (shouldLog(Level::Critical)) {
            getDefaultLogger().critical(fmt, args...);
        }
    }

    template <typename... Args>
    static void error(spdlog::string_view_t fmt, const Args&... args) {
        if (shouldLog(Level::Error)) {
            getDefaultLogger().error(fmt, args...);
        }
    }

    template <typename... Args>
    static void warn(spdlog::string_view_t fmt, const Args&... args) {
        if (shouldLog(Level::Warn)) {
            getDefaultLogger().warn(fmt, args...);
        }
    }

    template <typename... Args>
    static void info(spdlog::string_view_t fmt, const Args&... args) {
        if (shouldLog(Level::Info)) {
            getDefaultLogger().info(fmt, args...);
        }
    }

    template <typename... Args>
    static void debug(spdlog::string_view_t fmt, const Args&... args) {
        if (shouldLog(Level::Debug)) {
            getDefaultLogger().debug(fmt, args...);
        }
    }

    template <typename... Args>
    static void trace(spdlog::string_view_t fmt, const Args&... args) {
        if (shouldLog(Level::Trace)) {
            getDefaultLogger().trace(fmt, args...);
        }
    }

    /// Use this function to log messages that would normally be sent to
    /// std::cout. These messages always appear, and are also logged to the
    /// filesink (addFileSink()) and any sinks added via addSink().
    /// The main use case for this function is inside of functions whose intent
    /// is to print information (e.g., Component::printSubcomponentInfo()).
    /// Besides such use cases, this function should be used sparingly to
    /// give users control over what gets logged.
    template <typename... Args>
    static void cout(spdlog::string_view_t fmt, const Args&... args) {
        getCoutLogger().log(getCoutLogger().level(), fmt, args...);
    }

    /// @}

    /// Log messages to a file at the level getLevel().
    /// OpenSim logs messages to the file opensim.log by default.
    /// If we are already logging messages to a file, then this
    /// function issues a warning and returns; invoke removeFileSink() first.
    /// @note This function is not thread-safe. Do not invoke this function
    /// concurrently, or concurrently with addSink() or removeSink().
    /// @note If filepath can't be opened, no log file is created.
    static void addFileSink(const std::string& filepath = "opensim.log");

    /// Remove the filesink if it exists.
    /// If the filesink was already removed, then this does nothing.
    /// @note This function is not thread-safe. Do not invoke this function
    /// concurrently, or concurrently with addSink() or removeSink().
    static void removeFileSink();

    /// Start reporting messages to the provided sink.
    /// @note This function is not thread-safe. Do not invoke this function
    /// concurrently, or concurrently with addLogFile() or removeSink().
    static void addSink(const std::shared_ptr<LogSink> sink);

    /// Remove a sink. If it doesn't exist, do nothing.
    /// @note This function is not thread-safe. Do not invoke this function
    /// concurrently, or concurrently with addLogFile() or addSink().
    static void removeSink(const std::shared_ptr<LogSink> sink);
private:
    static spdlog::logger& getCoutLogger();
    static spdlog::logger& getDefaultLogger();
};

/// @name Logging functions
/// @{

/// @related Logger
template <typename... Args>
void log_critical(spdlog::string_view_t fmt, const Args&... args) {
    Logger::critical(fmt, args...);
}

/// @related Logger
template <typename... Args>
void log_error(spdlog::string_view_t fmt, const Args&... args) {
    Logger::error(fmt, args...);
}

/// @related Logger
template <typename... Args>
void log_warn(spdlog::string_view_t fmt, const Args&... args) {
    Logger::warn(fmt, args...);
}

/// @related Logger
template <typename... Args>
void log_info(spdlog::string_view_t fmt, const Args&... args) {
    Logger::info(fmt, args...);
}

/// @related Logger
template <typename... Args>
void log_debug(spdlog::string_view_t fmt, const Args&... args) {
    Logger::debug(fmt, args...);
}

/// @related Logger
template <typename... Args>
void log_trace(spdlog::string_view_t fmt, const Args&... args) {
    Logger::trace(fmt, args...);
}

/// @copydoc Logger::cout()
/// @related Logger
template <typename... Args>
void log_cout(spdlog::string_view_t fmt, const Args&... args) {
    Logger::cout(fmt, args...);
}

/// @}

} // namespace OpenSim

#endif // OPENSIM_LOG_H_
