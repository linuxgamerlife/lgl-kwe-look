#pragma once
#include <QDialog>

class QPlainTextEdit;

// Plain-text editor for one style sheet file.
class QssEditorDialog : public QDialog
{
    Q_OBJECT
public:
    QssEditorDialog(const QString &path, bool readOnly, QWidget *parent = nullptr);

    QString text() const;

private:
    QPlainTextEdit *m_edit = nullptr;
};
