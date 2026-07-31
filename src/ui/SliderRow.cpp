#include "ui/SliderRow.hpp"

#include <algorithm>
#include <cmath>

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>

namespace livim {

SliderRow::SliderRow(QWidget* parent) : QWidget(parent) {
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(10);

    slider_ = new QSlider(Qt::Horizontal, this);
    slider_->setMinimum(0);
    slider_->setMaximum(100);

    readout_ = new QLabel(this);
    readout_->setObjectName("valueReadout");
    readout_->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    readout_->setMinimumWidth(52);

    l->addWidget(slider_, 1);
    l->addWidget(readout_);

    connect(slider_, &QSlider::valueChanged, this, &SliderRow::onSliderMoved);

    rebuildSlider();
    updateReadout();
}

void SliderRow::rebuildSlider() {
    const double span = std::max(0.0, max_ - min_);
    const double stepSafe = step_ > 0.0 ? step_ : 1.0;
    const int ticks = std::max(1, static_cast<int>(std::lround(span / stepSafe)));
    updating_ = true;
    slider_->setMaximum(ticks);
    slider_->setValue(std::clamp(static_cast<int>(std::lround((value_ - min_) / stepSafe)), 0, ticks));
    updating_ = false;
}

void SliderRow::setRange(double min, double max) {
    min_ = min;
    max_ = max;
    value_ = std::clamp(value_, min_, max_);
    rebuildSlider();
    updateReadout();
}

void SliderRow::setSingleStep(double step) {
    step_ = step > 0.0 ? step : 1.0;
    rebuildSlider();
    updateReadout();
}

void SliderRow::setDecimals(int decimals) {
    decimals_ = std::max(0, decimals);
    updateReadout();
}

void SliderRow::setSuffix(const QString& suffix) {
    suffix_ = suffix;
    updateReadout();
}

void SliderRow::setValue(double v) {
    const double stepSafe = step_ > 0.0 ? step_ : 1.0;
    value_ = std::clamp(v, min_, max_);
    updating_ = true;
    slider_->setValue(std::clamp(static_cast<int>(std::lround((value_ - min_) / stepSafe)),
                                 0, slider_->maximum()));
    updating_ = false;
    updateReadout();
}

void SliderRow::setValueUser(double v) {
    const double before = value_;
    setValue(v);
    if (value_ != before) emit valueChanged(value_);
}

void SliderRow::onSliderMoved(int tick) {
    if (updating_) return;
    const double stepSafe = step_ > 0.0 ? step_ : 1.0;
    value_ = std::clamp(min_ + static_cast<double>(tick) * stepSafe, min_, max_);
    updateReadout();
    emit valueChanged(value_);
}

void SliderRow::setReadoutVisible(bool visible) { readout_->setVisible(visible); }

QString SliderRow::text() const { return readout_->text(); }

void SliderRow::updateReadout() {
    const QString t = QString::number(value_, 'f', decimals_) + suffix_;
    readout_->setText(t);
    emit textChanged(t);
}

} // namespace livim
