#pragma once

#include "natalie/forward.hpp"
#include "natalie/macros.hpp"
#include "natalie/types.hpp"
#include "tm/string.hpp"
#include "tm/vector.hpp"
#include <chrono>
#include <ctime>
#include <stdio.h>

namespace Natalie {

class NativeProfiler;

class NativeProfilerEvent {
public:
    enum class Type {
        SEND,
        PUBLIC_SEND,
        GC,
        ALLOCATE,
        RUNTIME,
    };

    static NativeProfilerEvent *named(Type type, const char *name);
    static NativeProfilerEvent *named(Type, TM::String &);
    NativeProfilerEvent *start_now();
    NativeProfilerEvent *start(std::chrono::time_point<std::chrono::system_clock>);
    NativeProfilerEvent *end_now();
    NativeProfilerEvent *end(std::chrono::time_point<std::chrono::system_clock>);

private:
    NativeProfilerEvent() = delete;
    NativeProfilerEvent(const NativeProfilerEvent::Type type, const TM::String name, const int tid)
        : m_type(type)
        , m_name(name)
        , m_tid(tid) { }

    const char *google_trace_type() const {
        switch (m_type) {
        case Type::SEND:
            return "send";
        case Type::PUBLIC_SEND:
            return "public_send";
        case Type::GC:
        case Type::ALLOCATE:
        case Type::RUNTIME:
            return "system";
        };
        NAT_UNREACHABLE();
    }

    const NativeProfilerEvent::Type m_type;
    const TM::String m_name;
    int m_tid;
    std::time_t m_start;
    std::time_t m_end;

    friend class NativeProfiler;
};

class NativeProfiler {
public:
    static inline NativeProfiler *the() {
        if (s_instance) {
            return s_instance;
        }

        s_instance = new NativeProfiler(false, -1);
        return s_instance;
    }

    static inline void enable() {
        if (s_instance) {
            NAT_UNREACHABLE();
        }
        s_instance = new NativeProfiler(true, getpid());
    }

    void push(NativeProfilerEvent *);
    bool enabled() const;
    void dump();

private:
    NativeProfiler() = delete;
    NativeProfiler(const bool enable, const int pid)
        : m_is_enabled(enable)
        , m_pid(pid) { }

    inline static NativeProfiler *s_instance = nullptr;
    const bool m_is_enabled;
    const int m_pid;

    TM::Vector<NativeProfilerEvent *> m_events {};
};
}
