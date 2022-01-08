#pragma once

#include "natalie/string.hpp"
#include "natalie/types.hpp"
#include "tm/hashmap.hpp"
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

    static NativeProfilerEvent *named(const Type type, const String *name) {
        return named(type, name->c_str());
    }

    static NativeProfilerEvent *named(const Type type, const char *name) {
        return new NativeProfilerEvent(type, strdup(name));
    }

    NativeProfilerEvent *tid(int tid) {
        m_tid = tid;
        return this;
    }

    NativeProfilerEvent *start_now() {
        return start(std::chrono::system_clock::now());
    }

    NativeProfilerEvent *start(std::chrono::time_point<std::chrono::system_clock> start) {
        m_start = std::chrono::duration_cast<std::chrono::microseconds>(start.time_since_epoch()).count();
        return this;
    }

    NativeProfilerEvent *end_now() {
        return end(std::chrono::system_clock::now());
    }

    NativeProfilerEvent *end(std::chrono::time_point<std::chrono::system_clock> end) {
        m_end = std::chrono::duration_cast<std::chrono::microseconds>(end.time_since_epoch()).count();
        return this;
    }

private:
    NativeProfilerEvent() = delete;
    NativeProfilerEvent(const NativeProfilerEvent::Type type, const char *name)
        : m_type(type)
        , m_name(name) { }

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
    const char *m_name;
    int m_tid;
    std::time_t m_start;
    std::time_t m_end;

    friend class NativeProfiler;
};

class NativeProfiler {
public:
    static NativeProfiler *the() {
        if (s_instance) {
            return s_instance;
        }

        s_instance = new NativeProfiler();
        return s_instance;
    }

    void push(NativeProfilerEvent *e) {
        if (!m_is_enabled)
            return;
        m_events.push(std::move(e));
    }

    bool enabled() {
        return m_is_enabled;
    }

    void enable(const char *name) {
        m_is_enabled = true;
        m_name = name;
        m_pid = getpid();
    }

    void dump() {
        if (!m_is_enabled)
            return;
        String path("profile-");
        path.append_sprintf("%lld", std::chrono::system_clock::now().time_since_epoch());
        path.append(".json");

        FILE *fp = fopen(path.c_str(), "w+");

        fprintf(fp, "[");
        for (size_t i = 0; i < m_events.size(); ++i) {
            auto *e = m_events[i];
            String event;
            if (i > 0)
                event.append_char(',');
            event.append_char('{');

            event.append_sprintf("\"name\":\"%s\"", e->m_name);
            event.append_sprintf(",\"cat\": \"%s\"", e->google_trace_type());
            event.append(",\"ph\": \"X\"");
            event.append_sprintf(",\"ts\": \"%lld\"", e->m_start);
            event.append_sprintf(",\"dur\": \"%lld\"", e->m_end - e->m_start);
            event.append_sprintf(",\"pid\": \"%d\"", m_pid);
            event.append_sprintf(",\"tid\": \"%d\"", e->m_tid);

            event.append_char('}');
            fprintf(fp, "%s", event.c_str());
        }
        fprintf(fp, "]");

        fclose(fp);
        printf("Profile path: %s\n", path.c_str());
    }

private:
    NativeProfiler() = default;

    inline static NativeProfiler *s_instance = nullptr;
    int m_pid;
    const char *m_name;
    bool m_is_enabled = false;

    TM::Vector<NativeProfilerEvent *> m_events {};
};
}
