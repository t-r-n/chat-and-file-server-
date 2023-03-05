#include "adddialog.h"

addDialog::addDialog(QWidget *parent) : QDialog(parent)
{
    line=new QLineEdit(this);
    setWindowTitle(tr("Add talkroom"));
    QHBoxLayout*hlayout=new QHBoxLayout();
    QHBoxLayout*hlayout1=new QHBoxLayout();
    QVBoxLayout*vlayout=new QVBoxLayout(this);
    auto okButton = new QPushButton(tr("OK"),this);
    auto cancelButton = new QPushButton(tr("Cancel"),this);
    hlayout->addWidget(okButton);
    hlayout->addWidget(cancelButton);
    //QLab
    hlayout1->addWidget(new QLabel("群号",this));
    hlayout1->addWidget(line);
    vlayout->addLayout(hlayout1);
    vlayout->addLayout(hlayout);
    connect(okButton, &QAbstractButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QAbstractButton::clicked, this, &QDialog::reject);
}

QString addDialog::id() const
{
    return line->text();
}


