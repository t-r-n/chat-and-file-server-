#ifndef ADDDIALOG_H
#define ADDDIALOG_H

#include <QWidget>
#include<QLineEdit>
#include<QDialog>
#include<QPushButton>
#include<QAbstractButton>
#include<QHBoxLayout>
#include<QVBoxLayout>
#include<QButtonGroup>
class addDialog : public QDialog
{
    Q_OBJECT
public:
    explicit addDialog(QWidget *parent = nullptr);
    QString id() const;
signals:
private:
    QLineEdit*line;
};

#endif // ADDDIALOG_H
