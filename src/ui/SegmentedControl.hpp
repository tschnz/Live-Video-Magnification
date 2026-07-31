#pragma once

#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>

namespace livim {

// Horizontal pill of mutually-exclusive segments (e.g. the 1/1 - 1/8 resolution divisors).
class SegmentedControl : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public:
    explicit SegmentedControl(QWidget* parent = nullptr);

    void addSegment(const QString& text);
    int  count() const { return static_cast<int>(segments_.size()); }

    int  currentIndex() const { return current_; }
    void setCurrentIndex(int index);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void currentIndexChanged(int index);

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    bool event(QEvent* e) override;

private:
    int  segmentAt(const QPoint& pt) const; // -1 if outside the segments
    QRectF segmentRect(int index) const;

    QStringList segments_;
    int         current_ = -1;
    int         hovered_ = -1;
};

} // namespace livim
