#include "ui/CameraSelectDialog.hpp"

#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace livim {

CameraSelectDialog::CameraSelectDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Open Camera");
    setMinimumWidth(360);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Select a camera:", this));

    list_ = new QListWidget(this);
    layout->addWidget(list_);

    auto* buttons = new QDialogButtonBox(this);
    okButton_ = buttons->addButton("Open", QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    auto* refreshButton = buttons->addButton("Refresh", QDialogButtonBox::ActionRole);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &CameraSelectDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &CameraSelectDialog::reject);
    connect(refreshButton, &QPushButton::clicked, this, &CameraSelectDialog::refresh);
    connect(list_, &QListWidget::itemDoubleClicked, this, &CameraSelectDialog::accept);
    connect(list_, &QListWidget::itemSelectionChanged, this, &CameraSelectDialog::updateOkEnabled);

    refresh();
}

void CameraSelectDialog::refresh() {
    devices_ = enumerateCameras();
    list_->clear();
    for (const CameraDevice& d : devices_)
        list_->addItem(QString::fromStdString(d.name));

    if (devices_.empty()) {
        auto* item = new QListWidgetItem("No cameras found");
        item->setFlags(Qt::NoItemFlags); // non-selectable placeholder
        list_->addItem(item);
    } else {
        list_->setCurrentRow(0);
    }
    updateOkEnabled();
}

void CameraSelectDialog::updateOkEnabled() {
    okButton_->setEnabled(!devices_.empty() && list_->currentRow() >= 0 &&
                          list_->currentRow() < static_cast<int>(devices_.size()));
}

void CameraSelectDialog::accept() {
    const int row = list_->currentRow();
    if (row < 0 || row >= static_cast<int>(devices_.size())) return;
    selectedIndex_ = devices_[row].index;
    selectedName_ = QString::fromStdString(devices_[row].name);
    QDialog::accept();
}

} // namespace livim
