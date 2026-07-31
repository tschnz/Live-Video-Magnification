#include "ui/SourceControlsView.hpp"

#include <QEvent>
#include <QPushButton>
#include <QSize>

namespace livim {

QPushButton* SourceControlsView::makeTransportButton() {
    auto* b = new QPushButton(this);
    b->setObjectName("transportBtn");
    b->setFixedSize(kTransportButtonW, kTransportButtonH);
    b->setIconSize(QSize(16, 16));
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

void SourceControlsView::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) refreshIcons();
}

} // namespace livim
