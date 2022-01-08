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
    static NativeProfilerEvent *named(const String *name) {
        return new NativeProfilerEvent(strdup(name->c_str()));
    }

    NativeProfilerEvent *tid(int tid) {
        m_tid = tid;
        return this;
    }

    NativeProfilerEvent *start(std::chrono::time_point<std::chrono::system_clock> start) {
        m_start = std::chrono::duration_cast<std::chrono::microseconds>(start.time_since_epoch()).count();
        return this;
    }

    NativeProfilerEvent *end(std::chrono::time_point<std::chrono::system_clock> end) {
        m_end = std::chrono::duration_cast<std::chrono::microseconds>(end.time_since_epoch()).count();
        return this;
    }

private:
    NativeProfilerEvent() = default;
    NativeProfilerEvent(const char *name)
        : m_name(name) { }

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
        path.append_sprintf("%ld", std::chrono::system_clock::now().time_since_epoch());
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
            event.append(",\"cat\": \"send\"");
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
