#pragma once

#include <QAbstractButton>
#include <QSize>

class QPropertyAnimation;

namespace livim {

// iOS/macOS-style pill toggle: a checkable QAbstractButton with an animated sliding knob.
class ToggleSwitch : public QAbstractButton {
    Q_OBJECT
    // 0 = knob fully left (off), 1 = fully right (on). Animated; not the logical state.
    Q_PROPERTY(qreal position READ position WRITE setPosition)

public:
    explicit ToggleSwitch(QWidget* parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

    qreal position() const { return pos_; }
    void  setPosition(qreal p);

protected:
    void paintEvent(QPaintEvent* e) override;
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    void animateTo(bool on);

    qreal              pos_ = 0.0;       // animated knob position, [0, 1]
    bool               hovered_ = false;
    QPropertyAnimation* anim_ = nullptr;
};

} // namespace livim
