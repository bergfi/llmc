#pragma once

struct TransitionInfoUnExpanded {

    static TransitionInfoUnExpanded None() { return TransitionInfoUnExpanded(0ULL); }

    template<typename T>
    static TransitionInfoUnExpanded construct(T& t) {
        static_assert(sizeof(t) == sizeof(_data));
        return TransitionInfoUnExpanded(*reinterpret_cast<decltype(_data)*>(&t));
    }

    template<typename T>
    T const& get() const {
        return *reinterpret_cast<const T*>(&_data);
    }
private:
    explicit TransitionInfoUnExpanded(size_t data): _data(data) {}
private:
    size_t _data;
};
