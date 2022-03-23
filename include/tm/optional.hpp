#pragma once

#include <assert.h>
#include <stdio.h>

namespace TM {

template <typename T>
class Optional {
public:
    /**
     * Constructs a new Optional with a value.
     *
     * ```
     * auto obj = Thing(1);
     * auto opt = Optional<Thing>(obj);
     * assert(opt);
     * ```
     */
    Optional(T value)
        : m_present { true }
        , m_value { value } { }

    /**
     * Constructs a new Optional without a value.
     *
     * ```
     * auto opt = Optional<Thing>();
     * assert_not(opt);
     * ```
     */
    Optional()
        : m_present { false } { }

    /**
     * Copies the given Optional.
     *
     * ```
     * auto obj = Thing(1);
     * auto opt1 = Optional<Thing>(obj);
     * auto opt2 = Optional<Thing>(opt1);
     * assert_eq(obj, opt1.value());
     * assert_eq(obj, opt2.value());
     * ```
     */
    Optional(const Optional &other)
        : m_present { other.m_present } {
        if (m_present)
            m_value = other.m_value;
    }

    ~Optional() {
        clear();
    }

    /**
     * Overwrites this Optional with the given one.
     *
     * ```
     * auto obj1 = Thing(1);
     * auto obj2 = Thing(2);
     * auto opt1 = Optional<Thing>(obj1);
     * auto opt2 = Optional<Thing>(obj2);
     * opt1 = opt2;
     * assert_eq(obj2, opt1.value());
     * assert_eq(obj2, opt2.value());
     * ```
     */
    Optional<T> &operator=(const Optional<T> &other) {
        m_present = other.m_present;
        if (m_present)
            m_value = other.m_value;
        return *this;
    }

    /**
     * Overwrites this Optional with the given moved value.
     *
     * ```
     * auto obj1 = Thing(1);
     * auto obj2 = Thing(2);
     * auto opt = Optional<Thing>(obj1);
     * opt = std::move(obj2);
     * assert_eq(obj2, opt.value());
     * ```
     */
    Optional<T> &operator=(T &&value) {
        m_present = true;
        m_value = value;
        return *this;
    }

    /**
     * Returns a reference to the underlying value.
     *
     * ```
     * auto obj = Thing(1);
     * auto opt = Optional<Thing>(obj);
     * assert_eq(obj, opt.value());
     * ```
     *
     * This method aborts if the value not present.
     *
     * ```should_abort
     * auto opt = Optional<Thing>();
     * opt.value();
     * ```
     */
    T &value() {
        assert(m_present);
        return m_value;
    }

    /**
     * Returns a reference to the underlying value.
     *
     * ```
     * auto obj = Thing(1);
     * auto opt = Optional<Thing>(obj);
     * assert_eq(obj, opt.value());
     * ```
     *
     * This method aborts if the value not present.
     *
     * ```should_abort
     * auto opt = Optional<Thing>();
     * opt.value();
     * ```
     */
    T const &value() const {
        assert(m_present);
        return m_value;
    }

    /**
     * Returns a reference to the underlying value or
     * the given fallback value.
     *
     * ```
     * auto obj = Thing(1);
     * auto opt = Optional<Thing>(obj);
     * assert_eq(Thing(1), opt.value_or(Thing(2)));
     * ```
     *
     * If we don't have a value present, then the fallback
     * is returned.
     *
     * ```
     * auto opt = Optional<Thing>();
     * assert_eq(Thing(2), opt.value_or(Thing(2)));
     * ```
     */
    T const &value_or(const T &fallback) const {
        if (present())
            return value();
        else
            return fallback;
    }

    /**
     * Returns a reference to the underlying value.
     *
     * ```
     * auto obj = Thing(1);
     * auto opt = Optional<Thing>(obj);
     * assert_eq(obj, *opt);
     * ```
     *
     * This method aborts if the value not present.
     *
     * ```should_abort
     * auto opt = Optional<Thing>();
     * *opt;
     * ```
     */
    T operator*() {
        assert(m_present);
        return m_value;
    }

    /**
     * Sets the Optional to not present.
     *
     * ```
     * auto opt = Optional<Thing>(Thing(1));
     * assert(opt);
     * opt.clear();
     * assert_not(opt);
     * ```
     */
    void clear() { m_present = false; }

    /**
     * Returns true if the Optional contains a value.
     *
     * ```
     * auto opt1 = Optional<Thing>(Thing(1));
     * auto opt2 = Optional<Thing>();
     * assert(opt1);
     * assert_not(opt2);
     * ```
     */
    operator bool() const { return m_present; }

    /**
     * Returns true if the Optional contains a value.
     *
     * ```
     * auto opt1 = Optional<Thing>(Thing(1));
     * auto opt2 = Optional<Thing>();
     * assert(opt1.present());
     * assert_not(opt2.present());
     * ```
     */
    bool present() const { return m_present; }

private:
    bool m_present;

    T m_value {};
};

}
