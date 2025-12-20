#pragma once

#include <assert.h>
#include <functional>
#include <string.h>
#include <utility>

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
    Optional(const T &value)
        : m_present { true } {
        new (m_value) T { value };
    }

    /**
     * Constructs a new Optional with a value.
     *
     * ```
     * auto obj = Thing(1);
     * auto opt = Optional<Thing>(std::move(obj));
     * assert(opt);
     * ```
     */
    Optional(T &&value)
        : m_present { true } {
        new (m_value) T { std::move(value) };
    }

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
            *reinterpret_cast<T *>(m_value) = *reinterpret_cast<const T *>(other.m_value);
    }

    /**
     * Moves the given Optional.
     *
     * ```
     * auto obj = Thing(1);
     * auto opt1 = Optional<Thing>(obj);
     * auto opt2 = Optional<Thing>(std::move(opt1));
     * assert_not(opt1);
     * assert_eq(obj, opt2.value());
     * ```
     */
    Optional(Optional &&other) {
        m_present = other.m_present;
        if (m_present) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                memcpy(m_value, other.m_value, sizeof(m_value));
                other.clear_after_trivial_copy();
            } else {
                *reinterpret_cast<T *>(m_value) = std::move(*reinterpret_cast<T *>(other.m_value));
                other.clear();
            }
        }
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
        if (this == &other)
            return *this;
        if (m_present)
            clear();
        m_present = other.m_present;
        if (m_present)
            *reinterpret_cast<T *>(m_value) = *reinterpret_cast<const T *>(other.m_value);
        return *this;
    }

    /**
     * Overwrites this Optional with the given one.
     *
     * ```
     * auto obj1 = Thing(1);
     * auto obj2 = Thing(2);
     * auto opt1 = Optional<Thing>(obj1);
     * auto opt2 = Optional<Thing>(obj2);
     * opt1 = std::move(opt2);
     * assert_eq(obj2, opt1.value());
     * assert_not(opt2);
     * ```
     */
    Optional<T> &operator=(Optional<T> &&other) {
        if (this == &other)
            return *this;
        if (m_present)
            clear();
        m_present = other.m_present;
        if (m_present) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                memcpy(m_value, other.m_value, sizeof(m_value));
                other.clear_after_trivial_copy();
            } else {
                *reinterpret_cast<T *>(m_value) = std::move(*reinterpret_cast<const T *>(other.m_value));
                other.clear();
            }
        }
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
     * assert_eq(Thing(2), opt.value());
     * ```
     */
    Optional<T> &operator=(T &&value) {
        if (m_present)
            clear();
        m_present = true;
        *reinterpret_cast<T *>(m_value) = std::move(value);
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
        return *reinterpret_cast<T *>(m_value);
    }

    /**
     * Returns a reference to the underlying value.
     *
     * ```
     * auto obj = Thing(1);
     * const auto opt = Optional<Thing>(obj);
     * assert_eq(obj, opt.value());
     * ```
     *
     * This method aborts if the value not present.
     *
     * ```should_abort
     * const auto opt = Optional<Thing>();
     * opt.value();
     * ```
     */
    T const &value() const {
        assert(m_present);
        return *reinterpret_cast<const T *>(m_value);
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
     * Returns a copy of the underlying value or
     * the result of calling the fallback lambda.
     *
     * ```
     * auto obj = Thing(1);
     * auto opt = Optional<Thing>(obj);
     * assert_eq(Thing(1), opt.value_or([]() { return Thing(2); }));
     * ```
     *
     * If we don't have a value present, then the fallback
     * is called and the result returned.
     *
     * ```
     * auto opt = Optional<Thing>();
     * assert_eq(Thing(2), opt.value_or([]() { return Thing(2); }));
     * ```
     */
    T value_or(std::function<T()> fallback) const {
        if (present())
            return *reinterpret_cast<const T *>(m_value);
        else
            return fallback();
    }

    /**
     * Returns a reference to the underlying value.
     *
     * ```
     * auto obj = Thing(1);
     * auto opt = Optional<Thing>(obj);
     * assert_eq(obj, *opt);
     *
     * *opt = Thing(2);
     * assert_eq(2, opt->value());
     * ```
     *
     * This method aborts if the value not present.
     *
     * ```should_abort
     * auto opt = Optional<Thing>();
     * *opt;
     * ```
     */
    T &operator*() {
        assert(m_present);
        return *reinterpret_cast<T *>(m_value);
    }

    /**
     * Returns a reference to the underlying value.
     *
     * ```
     * auto obj = Thing(1);
     * const auto opt = Optional<Thing>(obj);
     * assert_eq(obj, *opt);
     * ```
     *
     * This method aborts if the value not present.
     *
     * ```should_abort
     * const auto opt = Optional<Thing>();
     * *opt;
     * ```
     */
    T const &operator*() const {
        assert(m_present);
        return *reinterpret_cast<const T *>(m_value);
    }

    /**
     * Calls a method on the underlying value.
     *
     * ```
     * auto obj = Thing(1);
     * auto opt = Optional<Thing>(obj);
     * assert_eq(1, opt->value());
     * ```
     *
     * This method aborts if the value not present.
     *
     * ```should_abort
     * auto opt = Optional<Thing>();
     * opt->value();
     * ```
     */
    T *operator->() {
        assert(m_present);
        return reinterpret_cast<T *>(m_value);
    }

    /**
     * Calls a method on the underlying value.
     *
     * ```
     * auto obj = Thing(1);
     * const auto opt = Optional<Thing>(obj);
     * assert_eq(1, opt->value());
     * ```
     *
     * This method aborts if the value not present.
     *
     * ```should_abort
     * const auto opt = Optional<Thing>();
     * opt->value();
     * ```
     */
    T const *operator->() const {
        assert(m_present);
        return reinterpret_cast<const T *>(m_value);
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
    void clear() {
        if (m_present) {
            reinterpret_cast<T *>(m_value)->~T();
            memset(m_value, 0, sizeof(m_value));
        }
        m_present = false;
    }

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
    void clear_after_trivial_copy() {
        m_present = false;
        memset(m_value, 0, sizeof(m_value));
    }

    bool m_present;

    alignas(T) unsigned char m_value[sizeof(T)] {};
};

}
