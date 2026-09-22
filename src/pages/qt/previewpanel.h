#pragma once
#include <QPalette>
#include <QWidget>

class QStyle;

// A set of sample controls used to preview a style and a palette. The style and
// palette are applied to every widget in the subtree, never to the application, so
// the settings window keeps its own look.
class PreviewPanel : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewPanel(QWidget *parent = nullptr);

    void setPreviewStyle(QStyle *style);
    // Applies `palette` with the colours of `group` used for both Active and Inactive,
    // like qt5ct's preview, so the Disabled colours can be inspected on enabled widgets.
    void setPreviewPalette(const QPalette &palette, QPalette::ColorGroup group);
    // Style sheets are applied to the panel only.
    void setPreviewStyleSheet(const QString &sheet);

private:
    static void applyStyle(QWidget *w, QStyle *style);
    static void applyPalette(QWidget *w, const QPalette &palette);
};
