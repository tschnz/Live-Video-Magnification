#include "core/FramePool.hpp"

#include <cassert>

namespace livim {

FramePool::FramePool(std::size_t capacity)
    : core_(std::make_shared<Core>()), capacity_(capacity) {
    assert(capacity >= 1 && "FramePool capacity must be >= 1");
    core_->storage.reserve(capacity);
    core_->freeList.reserve(capacity);
    for (std::size_t i = 0; i < capacity; ++i) {
        auto f = std::make_unique<Frame>();
        core_->freeList.push_back(f.get());
        core_->storage.push_back(std::move(f));
    }
}

MutableFrameRef FramePool::acquire() {
    Frame* f = nullptr;
    {
        std::unique_lock<std::mutex> lk(core_->m);
        core_->cv.wait(lk, [&] { return !core_->freeList.empty() || core_->stopped; });
        if (core_->stopped) return nullptr;
        f = core_->freeList.back();
        core_->freeList.pop_back();
    }

    // The deleter keeps the core (which owns `f` via storage) alive and only returns the
    // pointer to the free list; it never deletes `f`.
    std::shared_ptr<Core> core = core_;
    return MutableFrameRef(f, [core](Frame* p) {
        std::lock_guard<std::mutex> lg(core->m);
        core->freeList.push_back(p);
        core->cv.notify_one();
    });
}

void FramePool::stop() {
    {
        std::lock_guard<std::mutex> lg(core_->m);
        core_->stopped = true;
    }
    core_->cv.notify_all();
}

void FramePool::reset() {
    std::lock_guard<std::mutex> lg(core_->m);
    core_->stopped = false;
    // In-flight frames rejoin the free list via their deleter; storage is not rebuilt.
}

} // namespace livim
