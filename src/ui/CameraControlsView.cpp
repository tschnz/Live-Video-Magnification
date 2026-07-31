#include "ui/CameraControlsView.hpp"

#include <QColor>
#include <QHBoxLayout>
#include <QPalette>
#include <QPushButton>

#include "ui/Icons.hpp"

namespace livim {

CameraControlsView::CameraControlsView(QWidget* parent) : SourceControlsView(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    playPauseBtn_ = makeTransportButton();
    playPauseBtn_->setProperty("accent", true);
    playPauseBtn_->setToolTip("Play / Pause");

    layout->addWidget(playPauseBtn_);
    layout->addStretch(1);

    connect(playPauseBtn_, &QPushButton::clicked, this, &SourceControlsView::playPauseRequested);

    refreshIcons();
}

void CameraControlsView::setPlaying(bool playing) {
    playing_ = playing;
    refreshIcons();
}

void CameraControlsView::refreshIcons() {
    const QColor onAccent = palette().color(QPalette::HighlightedText);
    playPauseBtn_->setIcon(playing_ ? icons::pause(onAccent, 16) : icons::play(onAccent, 16));
}

} // namespace livim
