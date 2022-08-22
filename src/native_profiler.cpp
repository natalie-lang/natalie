#include "natalie.hpp"

namespace Natalie {

NativeProfilerEvent *NativeProfilerEvent::named(Type type, const char *name) {
    auto str = String(name);
    return named(type, str);
}

NativeProfilerEvent *NativeProfilerEvent::named(Type type, String &name) {
#if defined(__OpenBSD__) or defined(__APPLE__)
    auto tid = 0; // FIXME: get thread id on OpenBSD/macOS
#else
    auto tid = gettid();
#endif
    return new NativeProfilerEvent(type, name, tid);
}

NativeProfilerEvent *NativeProfilerEvent::start_now() {
    return start(std::chrono::system_clock::now());
}

NativeProfilerEvent *NativeProfilerEvent::start(std::chrono::time_point<std::chrono::system_clock> start) {
    m_start = std::chrono::duration_cast<std::chrono::microseconds>(start.time_since_epoch()).count();
    return this;
}

NativeProfilerEvent *NativeProfilerEvent::end_now() {
    return end(std::chrono::system_clock::now());
}

NativeProfilerEvent *NativeProfilerEvent::end(std::chrono::time_point<std::chrono::system_clock> end) {
    m_end = std::chrono::duration_cast<std::chrono::microseconds>(end.time_since_epoch()).count();
    return this;
}

void NativeProfiler::push(NativeProfilerEvent *e) {
    if (!m_is_enabled)
        return;
    m_events.push(std::move(e));
}

bool NativeProfiler::enabled() const {
    return m_is_enabled;
}

void NativeProfiler::dump() {
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

        event.append_sprintf("\"name\":\"%s\"", e->m_name.c_str());
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
}
