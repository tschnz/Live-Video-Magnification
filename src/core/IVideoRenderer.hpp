#pragma once

namespace livim {

class LatestFrameMailbox;

// The pipeline's handle on the presentation widget. Lives in core/ (not ui/) so the Qt-free
// pipeline layer can depend on it. The renderer PULLS from the mailbox on its own present
// cadence; pixels never travel through Qt signals/slots.
class IVideoRenderer {
public:
    virtual ~IVideoRenderer() = default;

    virtual void bindMailbox(LatestFrameMailbox* mailbox) = 0;
};

} // namespace livim
