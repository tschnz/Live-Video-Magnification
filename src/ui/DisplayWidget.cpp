#include "ui/DisplayWidget.hpp"

#include <algorithm>
#include <cmath>

#include <QLabel>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QRubberBand>
#include <QTimer>

#include "core/Instrumentation.hpp"
#include "core/LatestFrameMailbox.hpp"

namespace livim {
namespace {

// Fullscreen quad, clip-space position + texcoord; v is flipped so cv::Mat row 0 maps to screen top.
constexpr float kQuad[] = {
    // x     y     u    v
    -1.f, -1.f, 0.f, 1.f,
     1.f, -1.f, 1.f, 1.f,
    -1.f,  1.f, 0.f, 0.f,
     1.f,  1.f, 1.f, 0.f,
};

constexpr char kVertexShader[] = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTex;
out vec2 vTex;
void main() {
    vTex = aTex;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

// Colour textures hold BGR bytes uploaded as RGB, so the shader outputs .bgr to swizzle back;
// gray is a single-channel GL_R8 texture replicated across RGB. Requires GLSL 330 core.
constexpr char kFragmentShader[] = R"(#version 330 core
in vec2 vTex;
out vec4 FragColor;
uniform sampler2D uTex;
uniform int uGrayscale; // 1 = single-channel texture, 0 = BGR-in-RGB texture
void main() {
    if (uGrayscale == 1) {
        float g = texture(uTex, vTex).r;
        FragColor = vec4(g, g, g, 1.0);
    } else {
        FragColor = vec4(texture(uTex, vTex).bgr, 1.0);
    }
}
)";

} // namespace

DisplayWidget::DisplayWidget(QWidget* parent) : QOpenGLWidget(parent) {
    presentTimer_ = new QTimer(this);
    presentTimer_->setInterval(8); // ~120 Hz poll; vsync caps actual present rate
    connect(presentTimer_, &QTimer::timeout, this, [this] { update(); });

    // Mouse-transparent so ROI drags pass through to the widget.
    for (QLabel*& lbl : paneLabel_) {
        lbl = new QLabel(this);
        lbl->setAttribute(Qt::WA_TransparentForMouseEvents);
        lbl->setStyleSheet("color: white; background-color: rgba(0,0,0,150);"
                           " padding: 2px 6px; border-radius: 3px; font-weight: bold;");
        lbl->hide();
    }
}

DisplayWidget::~DisplayWidget() {
    // GL deletes need a current context; context() is null if initializeGL never ran or after
    // context loss -- skip the deletes then (the driver already reclaimed the resources).
    if (context()) {
        makeCurrent();
        for (Tex* t : {&texProc_, &texOrig_}) {
            if (t->id != 0) {
                glDeleteTextures(1, &t->id);
                t->id = 0;
            }
        }
        vbo_.destroy();
        vao_.destroy();
        delete program_;
        program_ = nullptr;
        doneCurrent();
    }
}

void DisplayWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.f, 0.f, 0.f, 1.f);

    program_ = new QOpenGLShaderProgram(this);
    program_->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexShader);
    program_->addShaderFromSourceCode(QOpenGLShader::Fragment, kFragmentShader);
    program_->link();

    vao_.create();
    vao_.bind();

    vbo_.create();
    vbo_.bind();
    vbo_.allocate(kQuad, sizeof(kQuad));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));

    vbo_.release();
    vao_.release();

    for (Tex* t : {&texProc_, &texOrig_}) {
        glGenTextures(1, &t->id);
        glBindTexture(GL_TEXTURE_2D, t->id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    presentTimer_->start();
}

void DisplayWidget::resizeGL(int /*w*/, int /*h*/) {
    // Letterbox viewports are computed per-frame in paintGL from the current frame's aspect.
}

void DisplayWidget::uploadFrame(const Frame& f, Tex& tex) {
    const cv::Mat& src = f.image;

    const int channels = src.channels();
    const GLint internalFormat = (channels == 1) ? GL_R8 : GL_RGB8;
    const GLenum pixelFormat = (channels == 1) ? GL_RED : GL_RGB;

    glBindTexture(GL_TEXTURE_2D, tex.id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // Handle a non-contiguous cv::Mat (row padding) via row length in pixels.
    glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(src.step / src.elemSize()));

    if (f.width != tex.w || f.height != tex.h || channels != tex.channels) {
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, f.width, f.height, 0, pixelFormat,
                     GL_UNSIGNED_BYTE, src.data);
        tex.w = f.width;
        tex.h = f.height;
        tex.channels = channels;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, f.width, f.height, pixelFormat, GL_UNSIGNED_BYTE,
                        src.data);
    }

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DisplayWidget::drawTexture(const Tex& tex, int vx, int vy, int vw, int vh) {
    if (tex.w <= 0 || tex.h <= 0 || vw <= 0 || vh <= 0) return;

    const float frameAR = static_cast<float>(tex.w) / static_cast<float>(tex.h);
    const float regionAR = static_cast<float>(vw) / static_cast<float>(vh);
    int w = vw, h = vh;
    if (regionAR > frameAR) {
        h = vh;
        w = static_cast<int>(static_cast<float>(vh) * frameAR);
    } else {
        w = vw;
        h = static_cast<int>(static_cast<float>(vw) / frameAR);
    }
    glViewport(vx + (vw - w) / 2, vy + (vh - h) / 2, w, h);

    program_->bind();
    vao_.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    program_->setUniformValue("uTex", 0);
    program_->setUniformValue("uGrayscale", tex.channels == 1 ? 1 : 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    vao_.release();
    program_->release();
}

int DisplayWidget::layoutPanes(Pane panes[2]) const {
    const double W = width();
    const double H = height();
    switch (viewMode_) {
    case ViewMode::Processed:
        panes[0] = {QRectF(0, 0, W, H), false};
        return 1;
    case ViewMode::Original:
        panes[0] = {QRectF(0, 0, W, H), true};
        return 1;
    case ViewMode::SideBySide: {
        const double half = W * 0.5;
        panes[0] = {QRectF(0, 0, half, H), true};          // original = left
        panes[1] = {QRectF(half, 0, W - half, H), false};  // processed = right
        return 2;
    }
    case ViewMode::Stacked: {
        const double half = H * 0.5;
        panes[0] = {QRectF(0, 0, W, half), true};          // original = top
        panes[1] = {QRectF(0, half, W, H - half), false};  // processed = bottom
        return 2;
    }
    }
    return 0;
}

void DisplayWidget::paintGL() {
    const qreal dpr = devicePixelRatioF();
    const int fbW = static_cast<int>(width() * dpr);
    const int fbH = static_cast<int>(height() * dpr);

    glViewport(0, 0, fbW, fbH);
    glClear(GL_COLOR_BUFFER_BIT);

    const auto presentable = [](const FrameRef& f) { return f && !f->image.empty(); };

    const DisplayFrameRef df = mailbox_ ? mailbox_->latest() : nullptr;
    if (df && presentable(df->processed)) {
        const Frame& proc = *df->processed;
        // Both frames are published together; one seq check keeps the panes in lockstep.
        if (proc.seq != lastSeq_) {
            const bool needProc = (viewMode_ != ViewMode::Original);
            const bool needOrig = (viewMode_ != ViewMode::Processed);
            if (needProc) uploadFrame(proc, texProc_);
            if (needOrig && presentable(df->original)) uploadFrame(*df->original, texOrig_);
            if (instr_) {
                if (lastSeq_ != kNoSeq && proc.seq > lastSeq_ + 1)
                    instr_->addDisplaySkipped(proc.seq - lastSeq_ - 1);
                instr_->onDisplayed();
            }
            lastSeq_ = proc.seq;
        }
    }

    Pane panes[2];
    const int n = layoutPanes(panes);
    for (int i = 0; i < n; ++i) {
        const Tex& t = panes[i].original ? texOrig_ : texProc_;
        if (t.w <= 0) continue;
        const QRectF& r = panes[i].region;
        const int vx = static_cast<int>(std::lround(r.x() * dpr));
        const int vw = static_cast<int>(std::lround(r.width() * dpr));
        const int vh = static_cast<int>(std::lround(r.height() * dpr));
        const int vy = static_cast<int>(std::lround(fbH - (r.y() + r.height()) * dpr)); // GL y up
        drawTexture(t, vx, vy, vw, vh);
    }

    // Reposition the labels only on a layout change; doing it per frame churns the widgets.
    const int texW = texProc_.w > 0 ? texProc_.w : texOrig_.w;
    const int texH = texProc_.h > 0 ? texProc_.h : texOrig_.h;
    if (width() != lblSigW_ || height() != lblSigH_ || texW != lblSigTexW_ ||
        texH != lblSigTexH_ || static_cast<int>(viewMode_) != lblSigMode_) {
        updateLabels();
        lblSigW_ = width();
        lblSigH_ = height();
        lblSigTexW_ = texW;
        lblSigTexH_ = texH;
        lblSigMode_ = static_cast<int>(viewMode_);
    }
}

void DisplayWidget::updateLabels() {
    Pane panes[2];
    const int n = layoutPanes(panes);
    for (int i = 0; i < 2; ++i) {
        QLabel* lbl = paneLabel_[i];
        if (!lbl) continue;
        const bool shown = (i < n) && ((panes[i].original ? texOrig_.w : texProc_.w) > 0);
        if (!shown) {
            lbl->hide();
            continue;
        }
        const QString text = panes[i].original ? QStringLiteral("Original")
                                               : QStringLiteral("Processed");
        if (lbl->text() != text) lbl->setText(text);
        lbl->adjustSize();
        const QRectF img = letterboxRect(panes[i].region);
        lbl->move(static_cast<int>(img.left()) + 6, static_cast<int>(img.top()) + 6);
        lbl->show();
        lbl->raise();
    }
}

void DisplayWidget::setViewMode(ViewMode mode) {
    if (viewMode_ == mode) return;
    viewMode_ = mode;
    // Force a re-upload on the next paint so switching works while paused.
    lastSeq_ = kNoSeq;
    update();
}

QRectF DisplayWidget::letterboxRect(const QRectF& region) const {
    // All panes share geometry; use whichever texture is loaded for the aspect.
    const int tw = texProc_.w > 0 ? texProc_.w : texOrig_.w;
    const int th = texProc_.h > 0 ? texProc_.h : texOrig_.h;
    if (tw <= 0 || th <= 0 || region.width() <= 0.0 || region.height() <= 0.0) return region;
    const double frameAR = static_cast<double>(tw) / static_cast<double>(th);
    const double regionAR = region.width() / region.height();
    double cw = region.width(), ch = region.height();
    if (regionAR > frameAR) { ch = region.height(); cw = ch * frameAR; }
    else { cw = region.width(); ch = cw / frameAR; }
    return QRectF(region.x() + (region.width() - cw) * 0.5,
                  region.y() + (region.height() - ch) * 0.5, cw, ch);
}

QRectF DisplayWidget::paneImageRect(const QPointF& p) const {
    Pane panes[2];
    const int n = layoutPanes(panes);
    for (int i = 0; i < n; ++i)
        if (panes[i].region.contains(p)) return letterboxRect(panes[i].region);
    return n > 0 ? letterboxRect(panes[0].region) : QRectF(0, 0, width(), height());
}

void DisplayWidget::setRoiDrawingEnabled(bool enabled) {
    roiDrawing_ = enabled;
    setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
    if (!enabled && rubberBand_) rubberBand_->hide();
}

void DisplayWidget::mousePressEvent(QMouseEvent* e) {
    if (!roiDrawing_ || e->button() != Qt::LeftButton || (texProc_.w <= 0 && texOrig_.w <= 0)) {
        QOpenGLWidget::mousePressEvent(e);
        return;
    }
    // Lock the drag to the pane it started in.
    roiDrawRect_ = paneImageRect(e->position());
    const QRectF c = roiDrawRect_;
    const QPointF p = e->position();
    roiOrigin_ = QPoint(static_cast<int>(std::clamp(p.x(), c.left(), c.right())),
                        static_cast<int>(std::clamp(p.y(), c.top(), c.bottom())));
    if (!rubberBand_) rubberBand_ = new QRubberBand(QRubberBand::Rectangle, this);
    rubberBand_->setGeometry(QRect(roiOrigin_, QSize()));
    rubberBand_->show();
}

void DisplayWidget::mouseMoveEvent(QMouseEvent* e) {
    if (!roiDrawing_ || !rubberBand_ || !rubberBand_->isVisible()) {
        QOpenGLWidget::mouseMoveEvent(e);
        return;
    }
    const QRectF c = roiDrawRect_;
    const QPointF p = e->position();
    const QPoint cur(static_cast<int>(std::clamp(p.x(), c.left(), c.right())),
                     static_cast<int>(std::clamp(p.y(), c.top(), c.bottom())));
    rubberBand_->setGeometry(QRect(roiOrigin_, cur).normalized());
}

void DisplayWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (!roiDrawing_ || e->button() != Qt::LeftButton || !rubberBand_) {
        QOpenGLWidget::mouseReleaseEvent(e);
        return;
    }
    const QRect band = rubberBand_->geometry();
    rubberBand_->hide(); // but stay armed; the Select ROI toggle owns drawing state
    const QRectF c = roiDrawRect_;
    if (c.width() < 1.0 || c.height() < 1.0) return;
    if (band.width() < 6 || band.height() < 6) return; // ignore stray clicks

    float x = static_cast<float>((band.left() - c.left()) / c.width());
    float y = static_cast<float>((band.top() - c.top()) / c.height());
    float w = static_cast<float>(band.width() / c.width());
    float h = static_cast<float>(band.height() / c.height());
    x = std::clamp(x, 0.0f, 1.0f);
    y = std::clamp(y, 0.0f, 1.0f);
    w = std::clamp(w, 0.0f, 1.0f - x);
    h = std::clamp(h, 0.0f, 1.0f - y);
    if (w <= 0.0f || h <= 0.0f) return;
    emit roiSelected(x, y, w, h);
}

} // namespace livim
