#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "core/AtomicConfig.hpp"
#include "core/Frame.hpp"
#include "core/PipelineTypes.hpp"
#include "processing/IProcessor.hpp"

namespace livim {

class LatestFrameMailbox;
class Instrumentation;

// Owns one thread that pulls frames in order from the source queue, runs them through an ordered
// list of IProcessors, and publishes to the display mailbox. May parallelize WITHIN a frame, but
// never pipelines frame N and N+1 across threads (would add latency and force reordering).
class ProcessingChain {
public:
    ProcessingChain(FrameQueue* in, LatestFrameMailbox* out, Instrumentation* instr,
                    AtomicConfig<ProcessorConfig>* config);
    ~ProcessingChain();

    void setProcessors(std::vector<std::unique_ptr<IProcessor>> procs);

    void start();
    void stop();

private:
    void run();

    FrameQueue* in_;
    LatestFrameMailbox* out_;
    Instrumentation* instr_;
    AtomicConfig<ProcessorConfig>* config_;

    std::vector<std::unique_ptr<IProcessor>> chain_;
    std::thread thread_;
    std::atomic<bool> stop_{false};
};

} // namespace livim
