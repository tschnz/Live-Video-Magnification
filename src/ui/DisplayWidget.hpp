#pragma once

#include <cstdint>

#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPoint>
#include <QRectF>

#include "core/Frame.hpp"
#include "core/IVideoRenderer.hpp"

class QTimer;
class QOpenGLShaderProgram;
class QRubberBand;
class QMouseEvent;
class QLabel;

namespace livim {

class LatestFrameMailbox;
class Instrumentation;

// Presents the latest {original, processed} pair pulled from the mailbox on a timer.
// Threading: the GL context is current only on the GUI thread (initializeGL/paintGL/resizeGL);
// the widget holds the shared_ptr during upload so the Mats stay alive.
class DisplayWidget : public QOpenGLWidget,
                      protected QOpenGLFunctions_3_3_Core,
                      public IVideoRenderer {
    Q_OBJECT
public:
    enum class ViewMode {
        Processed,
        Original,
        SideBySide,
        Stacked,
    };

    explicit DisplayWidget(QWidget* parent = nullptr);
    ~DisplayWidget() override;

    void bindMailbox(LatestFrameMailbox* mailbox) override { mailbox_ = mailbox; }

    void setInstrumentation(Instrumentation* instr) { instr_ = instr; }

    void setViewMode(ViewMode mode);

    // Arm ROI drawing: a left-button drag defines a rectangle and emits roiSelected.
    void setRoiDrawingEnabled(bool enabled);

signals:
    void roiSelected(float x, float y, float w, float h); // normalized [0,1] rect, image space

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    struct Tex {
        unsigned int id = 0; // GLuint
        int w = 0;
        int h = 0;
        int channels = 0;
    };

    struct Pane {
        QRectF region; // logical widget coords
        bool original; // true = original frame/label, false = processed
    };

    void uploadFrame(const Frame& f, Tex& tex);
    // Letterbox-fits `tex` into the framebuffer-pixel region [vx,vy,vw,vh].
    void drawTexture(const Tex& tex, int vx, int vy, int vw, int vh);

    int layoutPanes(Pane panes[2]) const; // fills 1-2 panes for the ViewMode; returns the count
    QRectF letterboxRect(const QRectF& region) const; // logical coords
    QRectF paneImageRect(const QPointF& p) const;
    void updateLabels();

    LatestFrameMailbox* mailbox_ = nullptr;
    Instrumentation* instr_ = nullptr;

    QTimer* presentTimer_ = nullptr;
    QOpenGLShaderProgram* program_ = nullptr;
    QOpenGLVertexArrayObject vao_;
    QOpenGLBuffer vbo_{QOpenGLBuffer::VertexBuffer};

    Tex texProc_;
    Tex texOrig_; // pre-magnification frame

    static constexpr std::uint64_t kNoSeq = ~0ull;
    std::uint64_t lastSeq_ = kNoSeq; // display-skip accounting, keyed on the processed frame

    ViewMode viewMode_ = ViewMode::Processed;

    // Pane labels are child widgets, avoiding a GL/QPainter glyph-atlas conflict.
    QLabel* paneLabel_[2] = {nullptr, nullptr};

    // Signature of the last layout the labels were positioned for.
    int lblSigW_ = -1, lblSigH_ = -1, lblSigTexW_ = -1, lblSigTexH_ = -1, lblSigMode_ = -1;

    bool roiDrawing_ = false;
    QRubberBand* rubberBand_ = nullptr;
    QPoint roiOrigin_;
    QRectF roiDrawRect_; // image rect of the pane where the current drag started
};

} // namespace livim
