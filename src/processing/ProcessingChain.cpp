#include "processing/ProcessingChain.hpp"

#include <chrono>
#include <utility>

#include "core/Instrumentation.hpp"
#include "core/LatestFrameMailbox.hpp"
#include "processing/ChainBuilder.hpp"

namespace livim {

ProcessingChain::ProcessingChain(FrameQueue* in, LatestFrameMailbox* out, Instrumentation* instr,
                                 AtomicConfig<ProcessorConfig>* config)
    : in_(in), out_(out), instr_(instr), config_(config) {}

ProcessingChain::~ProcessingChain() { stop(); }

void ProcessingChain::setProcessors(std::vector<std::unique_ptr<IProcessor>> procs) {
    chain_ = std::move(procs);
}

void ProcessingChain::start() {
    if (thread_.joinable()) return;
    stop_.store(false, std::memory_order_release);
    thread_ = std::thread([this] { run(); });
}

void ProcessingChain::stop() {
    stop_.store(true, std::memory_order_release);
    if (in_) in_->stop(); // unblock a pop() that is waiting for the next frame
    if (thread_.joinable()) thread_.join();
}

void ProcessingChain::run() {
    FrameRef in;
    while (!stop_.load(std::memory_order_acquire)) {
        if (!in_->pop(in)) break;

        const std::shared_ptr<const ProcessorConfig> cfg = config_->read();

        try {
            FrameRef original;
            const FrameRef cur = runChainOnce(chain_, in, *cfg, original);

            // Publish both in one object so the two panes are always the SAME frame.
            auto pair = std::make_shared<DisplayFrame>();
            pair->processed = cur;
            pair->original = original;
            out_->publish(std::move(pair));
        } catch (const std::exception&) {
            // A stage threw: don't let it std::terminate the app. Reset the stateful stages (a
            // mid-frame throw can leave temporal state half-updated) and publish the input frame.
            if (instr_) instr_->onProcessingError();
            for (auto& p : chain_) p->reset();
            auto pair = std::make_shared<DisplayFrame>();
            pair->processed = in;
            pair->original = in;
            out_->publish(std::move(pair));
        } catch (...) {
            if (instr_) instr_->onProcessingError();
            for (auto& p : chain_) p->reset();
        }

        if (instr_) {
            instr_->onProcessed();
            const double ms =
                std::chrono::duration<double, std::milli>(now() - in->captureTs).count();
            instr_->recordLatency(ms);
        }
    }
}

} // namespace livim
