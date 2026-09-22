#pragma once
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QClipboard>
#include <QFontDialog>
#include <QFrame>
#include <QFutureWatcher>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QMimeData>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizeGrip>
#include <QSizePolicy>
#include <QSplitter>
#include <QSplitterHandle>
#include <QString>
#include <QThread>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWhatsThis>
#include <QWheelEvent>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include <functional>

// Shared UI helpers, ported from lgl-system-loadout's pagehelpers.h.

// Badge colours are kept from the benchmark; every other colour comes from the palette.
inline QString badgeStyle(bool ok)
{
    return ok ? QStringLiteral("color: #3db03d; font-weight: bold; font-size: 8pt;")
              : QStringLiteral("color: #cc7700; font-weight: bold; font-size: 8pt;");
}

// Copies text to the clipboard, setting both Clipboard and Selection modes
// for reliable behaviour across Wayland and X11.
inline void copyToClipboard(const QString &text)
{
    auto *md = new QMimeData;
    md->setText(text);
    QApplication::clipboard()->setMimeData(md, QClipboard::Clipboard);
    if (QApplication::clipboard()->supportsSelection()) {
        auto *mdSel = new QMimeData;
        mdSel->setText(text);
        QApplication::clipboard()->setMimeData(mdSel, QClipboard::Selection);
    }
}

// ---- Smooth scrolling QScrollArea ----
// Installs an event filter on every descendant so wheel events are always
// caught regardless of which child widget the cursor is over. Do not put a page
// with its own scrolling list (theme lists, QSS list) inside one of these.
class SmoothScrollArea : public QScrollArea
{
public:
    explicit SmoothScrollArea(QWidget *parent = nullptr) : QScrollArea(parent)
    {
        setFocusPolicy(Qt::WheelFocus);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setWidgetResizable(true);
        setFrameShape(QFrame::NoFrame);
    }

    void setWidget(QWidget *w)
    {
        QScrollArea::setWidget(w);
        if (w)
            installOnAll(w);
    }

private:
    void installOnAll(QObject *obj)
    {
        obj->installEventFilter(this);
        for (QObject *child : obj->children())
            installOnAll(child);
    }

    void doScroll(QWheelEvent *we)
    {
        QScrollBar *vbar = verticalScrollBar();
        const int px = we->pixelDelta().y();
        if (px != 0) {
            vbar->setValue(vbar->value() - px);
        } else {
            const int delta = we->angleDelta().y();
            // One notch = 120 units; rounding division avoids silent truncation on odd deltas.
            vbar->setValue(vbar->value() - (delta * 60 + (delta >= 0 ? 60 : -60)) / 120);
        }
    }

protected:
    void wheelEvent(QWheelEvent *e) override
    {
        doScroll(e);
        e->accept();
    }

    bool eventFilter(QObject *obj, QEvent *e) override
    {
        if (e->type() == QEvent::Wheel) {
            // Dialogs and popups parented to a page (font dialog, combo-box lists) are windows of
            // their own: they scroll themselves.
            const auto *w = qobject_cast<QWidget *>(obj);
            if (w && w->window() != window())
                return QScrollArea::eventFilter(obj, e);
            doScroll(static_cast<QWheelEvent *>(e));
            e->accept();
            return true;
        }
        if (e->type() == QEvent::ChildAdded) {
            auto *ce = static_cast<QChildEvent *>(e);
            if (ce->child())
                installOnAll(ce->child());
        }
        return QScrollArea::eventFilter(obj, e);
    }
};

// ---- Setting explainers ----
// A small "?" next to a setting: hover for the explanation, click to keep it open.
inline QToolButton *makeHelpButton(QWidget *parent, const QString &text)
{
    // Rich text so the tooltip wraps instead of running across the screen.
    const QString rich = QStringLiteral("<p>%1</p>").arg(text.toHtmlEscaped());
    auto *b = new QToolButton(parent);
    b->setText(QStringLiteral("?"));
    b->setAutoRaise(true);
    b->setFixedSize(22, 22);
    b->setCursor(Qt::WhatsThisCursor);
    b->setFocusPolicy(Qt::TabFocus);
    b->setToolTip(rich);
    b->setWhatsThis(rich);
    b->setAccessibleName(QObject::tr("Help"));
    b->setAccessibleDescription(text);
    QObject::connect(b, &QToolButton::clicked, b, [b, rich]() {
        QWhatsThis::showText(b->mapToGlobal(QPoint(b->width(), b->height())), rich, b);
    });
    return b;
}

// A control followed by its help button, for a form row or a box layout.
inline QWidget *withHelp(QWidget *control, const QString &help)
{
    auto *row = new QWidget;   // reparented when the row is added to a layout
    auto *l = new QHBoxLayout(row);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(6);
    l->addWidget(control, 1);
    l->addWidget(makeHelpButton(row, help), 0, Qt::AlignVCenter);
    return row;
}

// ---- Resize grip ----
// Adds a grab bar in the bottom-right corner of a window whose layout is a box or a grid,
// so a window without frame decorations (a tiling session, for example) can still be resized.
inline void addSizeGrip(QWidget *window)
{
    auto *grip = new QSizeGrip(window);
    const Qt::Alignment corner = Qt::AlignRight | Qt::AlignBottom;
    if (auto *grid = qobject_cast<QGridLayout *>(window->layout()))
        grid->addWidget(grip, grid->rowCount(), qMax(0, grid->columnCount() - 1), corner);
    else if (auto *box = qobject_cast<QBoxLayout *>(window->layout()))
        box->addWidget(grip, 0, corner);
    else
        delete grip;
}

// ---- Font picker ----
// The stock font dialog is small: open it 2.5 times larger (never larger than the screen).
// The non-native dialog is used because a native one cannot be resized from here.
inline QFont pickFont(QWidget *parent, const QFont &initial, bool *ok)
{
    QFontDialog dlg(initial, parent);
    dlg.setOption(QFontDialog::DontUseNativeDialog);
    addSizeGrip(&dlg);

    QSize size = dlg.sizeHint() * 2.5;
    const QScreen *screen = parent ? parent->screen() : QGuiApplication::primaryScreen();
    if (screen)
        size = size.boundedTo(screen->availableGeometry().size() * 0.9);
    dlg.resize(size);

    const bool accepted = dlg.exec() == QDialog::Accepted;
    if (ok)
        *ok = accepted;
    return accepted ? dlg.selectedFont() : initial;
}

// ---- Splitter with a visible grip ----
// The stock handle is a hairline in most styles and hard to find. This one is wider, draws a line
// with three dots, and lights up under the mouse. Use it wherever two panes can be resized.
class GripHandle : public QSplitterHandle
{
public:
    GripHandle(Qt::Orientation orientation, QSplitter *parent) : QSplitterHandle(orientation, parent)
    {
        setAttribute(Qt::WA_Hover);   // repaint on enter and leave
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        QColor c = palette().color(QPalette::WindowText);
        c.setAlphaF(underMouse() ? 0.85f : 0.35f);
        p.setPen(Qt::NoPen);
        p.setBrush(c);

        const bool vertical = orientation() == Qt::Horizontal;   // a vertical bar between side-by-side panes
        const QPoint mid = rect().center();
        if (vertical)
            p.drawRect(QRect(mid.x(), 0, 1, height()));
        else
            p.drawRect(QRect(0, mid.y(), width(), 1));
        for (int i = -1; i <= 1; ++i) {
            const QPoint dot = vertical ? QPoint(mid.x() - 1, mid.y() + i * 6) : QPoint(mid.x() + i * 6, mid.y() - 1);
            p.drawRect(QRect(dot, QSize(3, 3)));
        }
    }
};

class GripSplitter : public QSplitter
{
public:
    explicit GripSplitter(Qt::Orientation orientation, QWidget *parent = nullptr) : QSplitter(orientation, parent)
    {
        setHandleWidth(9);
        setChildrenCollapsible(false);
    }

protected:
    QSplitterHandle *createHandle() override { return new GripHandle(orientation(), this); }
};

// ---- Async badge checker ----
// Runs a list of check functions on a worker thread, then calls `onDone(results)`
// on the main thread. Checks run sequentially: concurrent probe processes can
// interfere with each other.
inline void runChecksAsync(QObject *context,
                           QList<QPair<QString, std::function<bool()>>> checks,
                           std::function<void(QMap<QString, bool>)> onDone)
{
    auto *watcher = new QFutureWatcher<QMap<QString, bool>>();
    // If the context is destroyed (page navigation) before the future finishes,
    // the callback becomes a no-op rather than a use-after-free.
    QPointer<QObject> guard(context);
    QObject::connect(watcher, &QFutureWatcher<QMap<QString, bool>>::finished, context,
                     [watcher, onDone, guard]() {
                         if (guard)
                             onDone(watcher->result());
                         watcher->deleteLater();
                     });
    watcher->setFuture(QtConcurrent::run([checks]() -> QMap<QString, bool> {
        QMap<QString, bool> results;
        for (const auto &item : checks)
            results[item.first] = item.second();
        return results;
    }));
}

// Row with a check box and a green/orange [Installed]-style badge.
inline QCheckBox *makeItemRow(QWidget *parent, QLayout *layout, const QString &label, bool installed,
                              bool showBadge = true)
{
    auto *row = new QWidget(parent);
    auto *rowLay = new QHBoxLayout(row);
    rowLay->setContentsMargins(0, 0, 0, 0);
    rowLay->setSpacing(8);

    auto *cb = new QCheckBox(label, row);
    rowLay->addWidget(cb, 1);

    if (showBadge) {
        auto *badge = new QLabel(installed ? QObject::tr("[Installed]") : QObject::tr("[Not Installed]"), row);
        badge->setObjectName(QStringLiteral("badge"));
        badge->setMinimumWidth(110);
        badge->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        badge->setStyleSheet(badgeStyle(installed));
        rowLay->addWidget(badge);
    }

    layout->addWidget(row);
    return cb;
}

// Row with a plain label and a badge, for read-only status lines.
inline QLabel *makeStatusRow(QWidget *parent, QLayout *layout, const QString &label, bool ok,
                             const QString &okText, const QString &badText)
{
    auto *row = new QWidget(parent);
    auto *rowLay = new QHBoxLayout(row);
    rowLay->setContentsMargins(0, 0, 0, 0);
    rowLay->setSpacing(8);
    rowLay->addWidget(new QLabel(label, row), 1);

    auto *badge = new QLabel(ok ? okText : badText, row);
    badge->setObjectName(QStringLiteral("badge"));
    badge->setMinimumWidth(110);
    badge->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    badge->setStyleSheet(badgeStyle(ok));
    rowLay->addWidget(badge);

    layout->addWidget(row);
    return badge;
}

// Toolbar button helper - consistent sizing that adapts to theme font
inline QPushButton *makeToolbarBtn(const QString &text, QWidget *parent = nullptr)
{
    auto *btn = new QPushButton(text, parent);
    btn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    btn->setMinimumWidth(btn->fontMetrics().horizontalAdvance(text) + 24);
    return btn;
}

// Description label that reads well on both light and dark themes
inline QLabel *makeDescLabel(QWidget *parent, const QString &desc)
{
    auto *dl = new QLabel(QStringLiteral("    ") + desc, parent);
    dl->setWordWrap(true);
    QPalette pal = dl->palette();
    QColor c = pal.color(QPalette::WindowText);
    c.setAlphaF(0.55f);
    pal.setColor(QPalette::WindowText, c);
    dl->setPalette(pal);
    return dl;
}

// KDE-style page header: bold title, dimmed subtitle, separator line.
inline QWidget *makePageHeader(QWidget *parent, const QString &title, const QString &subtitle)
{
    auto *w = new QWidget(parent);
    auto *l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(4);

    auto *t = new QLabel(title, w);
    QFont f = t->font();
    f.setBold(true);
    f.setPointSize(f.pointSize() + 4);
    t->setFont(f);
    l->addWidget(t);

    if (!subtitle.isEmpty()) {
        auto *s = new QLabel(subtitle, w);
        s->setWordWrap(true);
        QPalette pal = s->palette();
        QColor c = pal.color(QPalette::WindowText);
        c.setAlphaF(0.7f);
        pal.setColor(QPalette::WindowText, c);
        s->setPalette(pal);
        l->addWidget(s);
    }

    auto *line = new QFrame(w);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    l->addWidget(line);
    return w;
}

// StyledPanel frame with a vertical layout, like the benchmark's info frames.
inline QFrame *makePanel(QWidget *parent, QVBoxLayout **layoutOut = nullptr)
{
    auto *frame = new QFrame(parent);
    frame->setFrameShape(QFrame::StyledPanel);
    auto *l = new QVBoxLayout(frame);
    if (layoutOut)
        *layoutOut = l;
    return frame;
}

inline QLabel *makeSectionTitle(QWidget *parent, const QString &text)
{
    auto *l = new QLabel(QStringLiteral("<b>%1</b>").arg(text.toHtmlEscaped()), parent);
    l->setTextFormat(Qt::RichText);
    return l;
}
