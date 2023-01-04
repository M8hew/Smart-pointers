#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

class BaseControlBlock {
public:
    virtual void IncrementShared() = 0;

    virtual void DecrementShared() = 0;

    virtual size_t GetSharedCount() = 0;

    virtual bool CanBlockBeDeleted() = 0;

    virtual ~BaseControlBlock() = default;
};

template <typename T>
class PtrControlBlock : public BaseControlBlock {
public:
    PtrControlBlock() : ptr_(nullptr), shared_counter_(0) {
    }

    PtrControlBlock(T* ptr) : ptr_(ptr), shared_counter_(1) {
    }

    void IncrementShared() override {
        ++shared_counter_;
    }

    void DecrementShared() override {
        --shared_counter_;
    }

    size_t GetSharedCount() override {
        return shared_counter_;
    }

    bool CanBlockBeDeleted() override {
        return shared_counter_ == 0;
    }

    ~PtrControlBlock() {
        if (ptr_) {
            delete ptr_;
        }
    }

private:
    T* ptr_;
    size_t shared_counter_;
};

template <typename T>
class MakeSharedControlBlock : public BaseControlBlock {
public:
    MakeSharedControlBlock() : shared_counter_(1) {
    }

    void IncrementShared() override {
        ++shared_counter_;
    }

    void DecrementShared() override {
        --shared_counter_;
    }

    size_t GetSharedCount() override {
        return shared_counter_;
    }

    bool CanBlockBeDeleted() override {
        return shared_counter_ == 0;
    }

    ~MakeSharedControlBlock() {
        Get()->~T();
    }

    auto GetStorageRef() {
        return &obj_;
    }

    T* Get() {
        return reinterpret_cast<T*>(&obj_);
    }

private:
    size_t shared_counter_;
    typename std::aligned_storage<sizeof(T), alignof(T)>::type obj_;
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() {
    }

    SharedPtr(std::nullptr_t) {
    }

    explicit SharedPtr(T* ptr) : obj_(ptr), block_(new PtrControlBlock(ptr)) {
    }

    SharedPtr(const SharedPtr& other) : obj_(other.obj_), block_(other.block_) {
        if (block_) {
            block_->IncrementShared();
        }
    }

    SharedPtr(SharedPtr&& other) noexcept {
        Swap(other);
    }

    SharedPtr(T* obj, BaseControlBlock* block) : obj_(obj), block_(block) {
    }

    template <typename Y>
    friend class SharedPtr;

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) : obj_(other.obj_), block_(other.block_) {
        if (block_) {
            block_->IncrementShared();
        }
    }

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) noexcept : obj_(other.obj_), block_(other.block_) {
        other.obj_ = nullptr;
        other.block_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) : obj_(ptr), block_(other.block_) {
        if (block_) {
            block_->IncrementShared();
        }
    }

    // control block of Y, observing T*
    template <typename Y>
    SharedPtr(Y* ptr) : obj_(ptr), block_(new PtrControlBlock<Y>(ptr)) {
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this == &other) {
            return *this;
        }
        Reset();
        obj_ = other.obj_;
        block_ = other.block_;
        if (block_) {
            block_->IncrementShared();
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Reset();
        Swap(other);
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        obj_ = nullptr;
        if (block_) {
            block_->DecrementShared();
            if (block_->CanBlockBeDeleted()) {
                delete block_;
            }
            block_ = nullptr;
        }
    }

    void Reset(T* ptr) {
        Reset();
        obj_ = ptr;
        block_ = new PtrControlBlock<T>(ptr);
    }

    template <typename Y>
    void Reset(Y* ptr) {
        Reset();
        obj_ = ptr;
        block_ = new PtrControlBlock<Y>(ptr);
    }

    void Swap(SharedPtr& other) {
        std::swap(obj_, other.obj_);
        std::swap(block_, other.block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return obj_;
    }

    T& operator*() const {
        return *Get();
    }

    T* operator->() const {
        return Get();
    }

    size_t UseCount() const {
        if (block_) {
            return block_->GetSharedCount();
        }
        return 0;
    }

    explicit operator bool() const {
        return block_ != nullptr;
    }

private:
    T* obj_ = nullptr;
    BaseControlBlock* block_ = nullptr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right);

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    MakeSharedControlBlock<T>* block_ptr = new MakeSharedControlBlock<T>;
    try {
        new (block_ptr->GetStorageRef()) T(std::forward<Args>(args)...);
    } catch (...) {
        delete block_ptr;
        throw;
    }
    return SharedPtr<T>(block_ptr->Get(), block_ptr);
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};
