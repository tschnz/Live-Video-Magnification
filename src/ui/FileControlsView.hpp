#pragma once

#include "ui/SourceControlsView.hpp"

class QPushButton;

namespace livim {

class FileControlsView : public SourceControlsView {
    Q_OBJECT
public:
    explicit FileControlsView(QWidget* parent = nullptr);

    void setPlaying(bool playing) override;

protected:
    void refreshIcons() override;

private:
    QPushButton* playPauseBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    bool         playing_ = false;
};

} // namespace livim
