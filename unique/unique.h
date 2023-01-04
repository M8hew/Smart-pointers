#pragma once

#include "compressed_pair.h"

#include <algorithm>
#include <cstddef>  // std::nullptr_t
#include <utility>

template <class T>
struct DefaultDeleter {
    DefaultDeleter() {
    }

    DefaultDeleter(auto&& obj) {
    }

    void operator()(T* ptr) noexcept {
        delete ptr;
    }
};

template <class T>
struct DefaultDeleter<T[]> {
    void operator()(T* ptr) noexcept {
        delete[] ptr;
    }
};

// Primary template
template <typename T, typename Deleter = DefaultDeleter<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : data_(ptr, Deleter()) {
    }

    UniquePtr(T* ptr, Deleter deleter) : data_(ptr, std::forward<Deleter>(deleter)) {
    }

    UniquePtr(UniquePtr&& other) noexcept {
        data_ = {nullptr, Deleter()};
        Swap(other);
    }

    template <class DerivedType, class DerivedDeleter>
    UniquePtr(UniquePtr<DerivedType, DerivedDeleter>&& other) noexcept {
        Reset(other.Release());
        GetDeleter() = std::forward<DerivedDeleter>(other.GetDeleter());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (&other == this) {
            return *this;
        }
        Reset();
        Swap(other);
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    template <class DerivedType, class DerivedDeleter>
    UniquePtr& operator=(UniquePtr<DerivedType, DerivedDeleter>&& other) noexcept {
        Reset(other.Release());
        GetDeleter() = std::forward<DerivedDeleter>(other.GetDeleter());
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        auto return_value = data_.GetFirst();
        data_.GetFirst() = nullptr;
        return return_value;
    }

    void Reset(T* ptr = nullptr) {
        CompressedPair<T*, Deleter> to_do_swap(ptr, Deleter());
        std::swap(data_, to_do_swap);
        if (to_do_swap.GetFirst()) {
            to_do_swap.GetSecond()(to_do_swap.GetFirst());
        }
    }

    void Swap(UniquePtr& other) {
        std::swap(data_, other.data_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return data_.GetFirst();
    }

    Deleter& GetDeleter() {
        return data_.GetSecond();
    }

    const Deleter& GetDeleter() const {
        return data_.GetSecond();
    }

    explicit operator bool() const {
        return data_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    T& operator*() const {
        return *data_.GetFirst();
    }

    T* operator->() const {
        return data_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> data_;
};

// Void template
template <typename Deleter>
class UniquePtr<void, Deleter> {
public:
    UniquePtr(void* ptr) : ptr_(ptr), del_(Deleter()) {
    }

    ~UniquePtr() {
        del_(ptr_);
    }

private:
    void* ptr_;
    Deleter del_;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : data_(ptr, Deleter()) {
    }

    UniquePtr(T* ptr, Deleter deleter) : data_(ptr, std::forward<Deleter>(deleter)) {
    }

    UniquePtr(UniquePtr&& other) noexcept {
        data_ = {nullptr, Deleter()};
        Swap(other);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (&other == this) {
            return *this;
        }
        Reset();
        Swap(other);
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        auto return_value = data_.GetFirst();
        data_.GetFirst() = nullptr;
        return return_value;
    }

    void Reset(T* ptr = nullptr) {
        CompressedPair<T*, Deleter> to_do_swap(ptr, Deleter());
        std::swap(data_, to_do_swap);
        if (to_do_swap.GetFirst()) {
            to_do_swap.GetSecond()(to_do_swap.GetFirst());
        }
    }

    void Swap(UniquePtr& other) {
        std::swap(data_, other.data_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return data_.GetFirst();
    }

    Deleter& GetDeleter() {
        return data_.GetSecond();
    }

    const Deleter& GetDeleter() const {
        return data_.GetSecond();
    }

    explicit operator bool() const {
        return data_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    T& operator[](size_t ind) const {
        return *(data_.GetFirst() + ind);
    }

private:
    CompressedPair<T*, Deleter> data_;
};
