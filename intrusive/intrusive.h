#pragma once

#include <cstddef>  // for std::nullptr_t
#include <utility>  // for std::exchange / std::swap

class SimpleCounter {
public:
    size_t IncRef() {
        return ++count_;
    }

    size_t DecRef() {
        return --count_;
    }

    size_t RefCount() const {
        return count_;
    }

private:
    size_t count_ = 0;
};

struct DefaultDelete {
    template <typename T>
    static void Destroy(T* object) {
        delete object;
    }
};

template <typename Derived, typename Counter, typename Deleter>
class RefCounted {
public:
    // Increase reference counter.
    void IncRef() {
        counter_.IncRef();
    }

    // Decrease reference counter.
    // Destroy object using Deleter when the last instance dies.
    void DecRef() {
        counter_.DecRef();
        if (!counter_.RefCount()) {
            Deleter::Destroy(static_cast<Derived*>(this));
        }
    }

    // Get current counter value (the number of strong references).
    size_t RefCount() const {
        return counter_.RefCount();
    }

    RefCounted& operator=(const RefCounted& other) {
        return *this;
    }

private:
    Counter counter_;
};

template <typename Derived, typename D = DefaultDelete>
using SimpleRefCounted = RefCounted<Derived, SimpleCounter, D>;

template <typename T>
class IntrusivePtr {
    template <typename Y>
    friend class IntrusivePtr;

public:
    // Constructors
    IntrusivePtr() {
    }

    IntrusivePtr(std::nullptr_t) {
    }

    IntrusivePtr(T* ptr) {
        Reset(ptr);
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y>& other) {
        Reset(other.obj_);
    }

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y>&& other) noexcept : obj_(other.obj_) {
        other.obj_ = nullptr;
    }

    IntrusivePtr(const IntrusivePtr& other) {
        Reset(other.obj_);
    }

    IntrusivePtr(IntrusivePtr&& other) noexcept {
        Swap(other);
    }

    // `operator=`-s
    IntrusivePtr& operator=(const IntrusivePtr& other) {
        if (this == &other) {
            return *this;
        }
        Reset(other.obj_);
        return *this;
    }

    IntrusivePtr& operator=(IntrusivePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Reset();
        Swap(other);
        return *this;
    }

    // Destructor
    ~IntrusivePtr() {
        Reset();
    }

    // Modifiers
    void Reset() {
        if (obj_) {
            obj_->DecRef();
            obj_ = nullptr;
        }
    }

    void Reset(T* ptr) {
        Reset();
        obj_ = ptr;
        if (obj_) {
            obj_->IncRef();
        }
    }

    void Swap(IntrusivePtr& other) {
        std::swap(obj_, other.obj_);
    }

    // Observers
    T* Get() const {
        return obj_;
    }

    T& operator*() const {
        return *obj_;
    }

    T* operator->() const {
        return obj_;
    }

    size_t UseCount() const {
        if (obj_) {
            return obj_->RefCount();
        }
        return 0;
    }

    explicit operator bool() const {
        return obj_;
    }

private:
    T* obj_ = nullptr;
};

template <typename T, typename... Args>
IntrusivePtr<T> MakeIntrusive(Args&&... args) {
    return IntrusivePtr<T>(new T(std::forward<Args>(args)...));
}
