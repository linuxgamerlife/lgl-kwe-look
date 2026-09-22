#include "qsseditordialog.h"

#include "pagehelpers.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QFontDatabase>
#include <QPlainTextEdit>
#include <QVBoxLayout>

QssEditorDialog::QssEditorDialog(const QString &path, bool readOnly, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(readOnly ? tr("View Style Sheet") : tr("Edit Style Sheet"));
    resize(640, 520);

    auto *layout = new QVBoxLayout(this);
    m_edit = new QPlainTextEdit(this);
    m_edit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_edit->setReadOnly(readOnly);
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
        m_edit->setPlainText(QString::fromUtf8(f.readAll()));
    layout->addWidget(m_edit);

    auto *buttons = new QDialogButtonBox(readOnly ? QDialogButtonBox::Close
                                                  : (QDialogButtonBox::Save | QDialogButtonBox::Cancel),
                                         this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    addSizeGrip(this);
}

QString QssEditorDialog::text() const
{
    return m_edit->toPlainText();
}
