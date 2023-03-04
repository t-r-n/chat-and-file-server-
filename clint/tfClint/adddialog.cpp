#include "adddialog.h"

addDialog::addDialog(QWidget *parent) : QDialog(parent)
{
    line=new QLineEdit(this);
    setWindowTitle(tr("Add talkroom"));
    QHBoxLayout*hlayout=new QHBoxLayout(this);
    QVBoxLayout*vlayout=new QVBoxLayout(this);
    auto okButton = new QPushButton(tr("OK"),this);
    auto cancelButton = new QPushButton(tr("Cancel"),this);
    hlayout->addWidget(okButton);
    hlayout->addWidget(cancelButton);
    vlayout->addWidget(line);
    vlayout->addLayout(hlayout);
    connect(okButton, &QAbstractButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QAbstractButton::clicked, this, &QDialog::reject);
}

QString addDialog::id() const
{
    return line->text();
}


