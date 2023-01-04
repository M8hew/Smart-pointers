#pragma once

#include <type_traits>
#include <utility>

template <class F, class S>
constexpr short CaseSwitch() {
    bool first_empty = std::is_empty_v<F> && !std::is_final_v<F>;
    bool second_empty = std::is_empty_v<S> && !std::is_final_v<S>;
    if (!first_empty && !second_empty) {
        return 1;
    }
    if (!first_empty) {
        return 2;
    }
    if (!second_empty) {
        return 3;
    }
    if (std::is_base_of_v<S, F> || std::is_base_of_v<F, S>) {
        return 1;
    }
    return 4;
}

// Me think, why waste time write lot code, when few code do trick.
template <class F, class S, short = CaseSwitch<F, S>()>
class CompressedPair {};

template <typename F, typename S>
class CompressedPair<F, S, 1> {
public:
    CompressedPair() : first_(), second_() {
    }

    template <typename T1, typename T2>
    CompressedPair(T1&& first, T2&& second)
        : first_(std::forward<T1>(first)), second_(std::forward<T2>(second)) {
    }

    F& GetFirst() {
        return first_;
    }

    const F& GetFirst() const {
        return first_;
    }

    S& GetSecond() {
        return second_;
    }

    const S& GetSecond() const {
        return second_;
    };

private:
    F first_;
    S second_;
};

template <typename F, typename S>
class CompressedPair<F, S, 2> : S {
public:
    CompressedPair() : first_() {
    }

    template <typename T1, typename T2>
    CompressedPair(T1&& first, T2&& second) : first_(std::forward<T1>(first)) {
    }

    F& GetFirst() {
        return first_;
    }

    const F& GetFirst() const {
        return first_;
    }

    S& GetSecond() {
        return static_cast<S&>(*this);
    }

    const S& GetSecond() const {
        return static_cast<const S&>(*this);
    };

private:
    F first_;
};

template <typename F, typename S>
class CompressedPair<F, S, 3> : F {
public:
    CompressedPair() : second_() {
    }

    template <typename T1, typename T2>
    CompressedPair(T1&& first, T2&& second) : second_(std::forward<T2>(second)) {
    }

    F& GetFirst() {
        return static_cast<F&>(*this);
    }

    const F& GetFirst() const {
        return static_cast<const F&>(*this);
    }

    S& GetSecond() {
        return second_;
    }

    const S& GetSecond() const {
        return second_;
    };

private:
    S second_;
};

template <typename F, typename S>
class CompressedPair<F, S, 4> : S, F {
public:
    template <typename T1, typename T2>
    CompressedPair(T1&& first, T2&& second) {
    }

    F& GetFirst() {
        return static_cast<F&>(*this);
    }

    const F& GetFirst() const {
        return static_cast<const F&>(*this);
    }

    S& GetSecond() {
        return static_cast<S&>(*this);
    }

    const S& GetSecond() const {
        return static_cast<const S&>(*this);
    };
};
