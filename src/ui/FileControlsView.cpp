#include "ui/FileControlsView.hpp"

#include <QColor>
#include <QHBoxLayout>
#include <QPalette>
#include <QPushButton>

#include "ui/Icons.hpp"

namespace livim {

FileControlsView::FileControlsView(QWidget* parent) : SourceControlsView(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    playPauseBtn_ = makeTransportButton();
    playPauseBtn_->setProperty("accent", true);
    playPauseBtn_->setToolTip("Play / Pause");

    stopBtn_ = makeTransportButton();
    stopBtn_->setToolTip("Stop");

    layout->addWidget(playPauseBtn_);
    layout->addWidget(stopBtn_);
    layout->addStretch(1);

    connect(playPauseBtn_, &QPushButton::clicked, this, &SourceControlsView::playPauseRequested);
    connect(stopBtn_, &QPushButton::clicked, this, &SourceControlsView::stopRequested);

    refreshIcons();
}

void FileControlsView::setPlaying(bool playing) {
    playing_ = playing;
    refreshIcons();
}

void FileControlsView::refreshIcons() {
    const QColor onAccent = palette().color(QPalette::HighlightedText);
    const QColor normal = palette().color(QPalette::ButtonText);
    playPauseBtn_->setIcon(playing_ ? icons::pause(onAccent, 16) : icons::play(onAccent, 16));
    if (stopBtn_) stopBtn_->setIcon(icons::stop(normal, 16));
}

} // namespace livim
