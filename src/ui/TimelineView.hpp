#pragma once

#include <cstdint>

#include <QWidget>

namespace livim {

// Frame-based scrub timeline with draggable IN/OUT handles bounding the active range [in, out).
// Time is derived from frame / playback fps, not from the file's native rate.
class TimelineView : public QWidget {
    Q_OBJECT
public:
    explicit TimelineView(QWidget* parent = nullptr);

    void setFrameCount(std::int64_t total);      // <= 0 disables interaction
    void setPlayheadFrame(std::int64_t frame);   // ignored while the user is scrubbing
    void setFps(double fps);
    void setInOut(std::int64_t in, std::int64_t out); // out exclusive; clamps to the clip
    void resetToStart();                         // snaps the playhead to the in-point

    std::int64_t frameCount() const { return total_; }
    std::int64_t inFrame() const { return in_; }
    std::int64_t outFrame() const { return out_; } // exclusive

    QSize sizeHint() const override;

signals:
    void seekRequested(std::int64_t frame);
    void scrubStarted();
    void scrubFinished();
    void inOutChanged(std::int64_t in, std::int64_t out);

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    enum class Drag { None, Playhead, In, Out };

    double       trackLeft() const;
    double       trackRight() const;
    int          frameToX(std::int64_t f) const;
    std::int64_t xToFrame(int x) const;
    std::int64_t clampToRange(std::int64_t f) const; // into [in, out)
    QString      timeText() const;

    std::int64_t total_ = 0;
    std::int64_t playhead_ = 0;
    std::int64_t in_ = 0;
    std::int64_t out_ = 0;   // exclusive; == total_ when the whole clip is selected
    double       fps_ = 30.0;
    Drag         drag_ = Drag::None;
};

} // namespace livim
