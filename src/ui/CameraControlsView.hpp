#pragma once

#include "ui/SourceControlsView.hpp"

class QPushButton;

namespace livim {

class CameraControlsView : public SourceControlsView {
    Q_OBJECT
public:
    explicit CameraControlsView(QWidget* parent = nullptr);

    void setPlaying(bool playing) override;

protected:
    void refreshIcons() override;

private:
    QPushButton* playPauseBtn_ = nullptr;
    bool         playing_ = false;
};

} // namespace livim
