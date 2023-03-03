#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void slot1();
    void slot2();

private:
    Ui::Dialog *ui;

    inline static const char *rqss =
        "background-color: %1;"
        "color: black;";

    QColor col = QColor(Qt::green);
    QString qss = QString(rqss).arg(col.name());
};
#endif // DIALOG_H
