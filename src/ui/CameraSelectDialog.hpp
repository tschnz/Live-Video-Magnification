#pragma once

#include <vector>

#include <QDialog>
#include <QString>

#include "source/CameraEnumerator.hpp"

class QListWidget;
class QPushButton;

namespace livim {

// Lists capture devices; selectedDeviceIndex() is the cv::VideoCapture ordinal.
class CameraSelectDialog : public QDialog {
    Q_OBJECT
public:
    explicit CameraSelectDialog(QWidget* parent = nullptr);

    int selectedDeviceIndex() const { return selectedIndex_; }     // -1 if none chosen
    QString selectedDeviceName() const { return selectedName_; }

private slots:
    void refresh();
    void accept() override;

private:
    void updateOkEnabled();

    QListWidget* list_ = nullptr;
    QPushButton* okButton_ = nullptr;
    std::vector<CameraDevice> devices_;
    int selectedIndex_ = -1;
    QString selectedName_;
};

} // namespace livim
