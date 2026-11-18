#ifndef OPENSIM_LOGSINK_H_
#define OPENSIM_LOGSINK_H_
/* -------------------------------------------------------------------------- *
 *                         OpenSim:  LogSink.h                                *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2026 Stanford University and the Authors                *
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

#include <OpenSim/Common/LogLevel.h>
#include <OpenSim/Common/LogMessage.h>
#include <OpenSim/Common/osimCommonDLL.h>

#include <string>

// This file is not included in osimCommon.h. Only include
// this file when deriving from LogSink.
namespace OpenSim {

/// Derive from this class to implement your own way of reporting logged
/// messages.
class OSIMCOMMON_API LogSink {
public:
    virtual ~LogSink() noexcept = default;

    /// Sinks `msg` into this `LogSink`.
    void sink(const LogMessage& msg) {
        sinkImpl(msg);
        sinkImpl(msg.getPayload());
    }

    /// Tells this `LogSink` to flush any buffered content to its output.
    void flush() { flushImpl(); }

    LogLevel getLevel() const { return level_; }
    void setLevel(LogLevel logLevel) { level_ = logLevel; }
    bool shouldLog(LogLevel logLevel) { return logLevel >= level_; }

protected:
    /// Implementors may override this function to provide their own
    /// message sinking behavior.
    virtual void sinkImpl(const LogMessage&) {}

    /// Implementors may override this function to provide their own
    /// message sinking behavior.
    ///
    /// Legacy shim: OpenSim 2019/11 to 2026/06 only provided this overload.
    virtual void sinkImpl(const std::string& msg) {}

    /// Implementors may override this function to provide their own
    /// message flushing behavior.
    virtual void flushImpl() {}

private:
    LogLevel level_ = LogLevel::Trace;
};

} // namespace OpenSim

#endif // OPENSIM_LOGSINK_H_
