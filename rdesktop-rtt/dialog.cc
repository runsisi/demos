#include "dialog.h"
#include "./ui_dialog.h"

#include <QDebug>
#include <QIntValidator>
#include <QRegExp>
#include <QMessageBox>

#include <chrono>
#include <string>

#include <sys/fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    Qt::WindowFlags flags = Qt::Window
        | Qt::WindowSystemMenuHint
        | Qt::WindowMinimizeButtonHint
        | Qt::WindowCloseButtonHint;
    this->setWindowFlags(flags);

    ui->setupUi(this);

    ui->tickBtn->setFixedSize(200, 200);
    ui->tickBtn->setStyleSheet(qss);
    QRect rect(QPoint(), ui->tickBtn->size());
    rect.adjust(10, 10, -10, -10);
    QRegion region(rect,QRegion::Ellipse);
    ui->tickBtn->setMask(region);

    QString ipRange = "^((25[0-5]|(2[0-4]|1\\d|[1-9]|)\\d)\\.?\\b){4}$";
    QRegExp ipRegex (ipRange);
    QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
    ui->ipEdit->setValidator(ipValidator);
    ui->ipEdit->setText("192.168.10.101");

    QValidator *validator = new QIntValidator(1, 65535, this);
    ui->portEdit->setValidator(validator);

    ui->portEdit->setText("3333");
}

Dialog::~Dialog()
{
    delete ui;
}

static int write_wait(int fd, int timeout) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLOUT;

    int r, err;
    do {
        r = poll(&pfd, 1, timeout);
        if (r < 0) {
            err = errno;
        }
    } while (r < 0 && err == EINTR);

    if (r < 0) {
        return -err;
    }

    if (r == 0) {
        err = ETIMEDOUT;
        return -err;
    }

    if (!(pfd.revents & POLLOUT)) {
        err = ECONNRESET;
        return -err;
    }

    return 0;
}

static int read_wait(int fd, int timeout) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    int r, err;
    do {
        r = poll(&pfd, 1, timeout);
        if (r < 0) {
            err = errno;
        }
    } while (r < 0 && err == EINTR);

    if (r < 0) {
        return -err;
    }

    if (r == 0) {
        err = ETIMEDOUT;
        return -err;
    }

    if (!(pfd.revents & POLLIN)) {
        err = ECONNRESET;
        return -err;
    }

    return 0;
}

void Dialog::slot1()
{
    namespace chrono = std::chrono;

    qDebug() << "request";

    int err = 0;

    if (!ui->ipEdit->hasAcceptableInput()) {
        QMessageBox::warning(NULL, "Warning", "invalid IP address");
        return;
    }
    if (!ui->portEdit->hasAcceptableInput()) {
        QMessageBox::warning(NULL, "Warning", "invalid port");
        return;
    }

    QString text_ip = ui->ipEdit->text();
    QString text_port = ui->portEdit->text();

    struct in_addr in_addr;
    int r = inet_aton(text_ip.toStdString().c_str(), &in_addr);
    if (!r) {
        QString info = QString("invalid ip address").arg(text_ip);
        QMessageBox::warning(NULL, "Warning", info);
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = in_addr.s_addr;

    try {
        short port = std::stoul(text_port.toStdString());
        addr.sin_port = htons(port);
    } catch (...) {
        QString info = QString("invalid port: %1").arg(text_port);
        QMessageBox::warning(NULL, "Warning", info);
        return;
    }

    auto now = chrono::steady_clock::now();

    // send request

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        err = errno;
        QString info = QString("create socket failed: %1").arg(strerror(err));
        QMessageBox::warning(NULL, "Warning", info);
        return;
    }

    r = ::fcntl(fd, F_GETFL, 0);
    if (r < 0) {
        err = errno;
        QString info = QString("fcntl F_GETFL failed: %1").arg(strerror(err));
        QMessageBox::warning(NULL, "Warning", info);
        return;
    }

    r = fcntl(fd, F_SETFL, r | O_NONBLOCK);
    if (r < 0) {
        err = errno;
        QString info = QString("fcntl F_SETFL failed: %1").arg(strerror(err));
        QMessageBox::warning(NULL, "Warning", info);
        return;
    }

    r = ::connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (r < 0) {
        err = errno;
        if (err != EWOULDBLOCK && err != EINPROGRESS) {
            QString info = QString("connect failed: %1").arg(strerror(err));
            QMessageBox::warning(NULL, "Warning", info);
            return;
        }
    }

    r = write_wait(fd, 1000);
    if (r < 0) {
        QString info = QString("connect failed: %1").arg(strerror(-r));
        QMessageBox::warning(NULL, "Warning", info);
        return;
    }

    char req[sizeof("REQUEST")] = "REQUEST";
    r = ::send(fd, req, sizeof(req), 0);
    if (r < 0) {
        err = errno;
        QString info = QString("send failed: %1").arg(strerror(err));
        QMessageBox::warning(NULL, "Warning", info);
        return;
    }

    r = read_wait(fd, 1000);
    if (r < 0) {
        QString info = QString("recv failed: %1").arg(strerror(-r));
        QMessageBox::warning(NULL, "Warning", info);
        return;
    }

    // recv response
    char resp[sizeof("RESPONSE")] = {0};
    r = ::recv(fd, resp, sizeof(resp), 0);
    if (r < 0) {
        err = errno;
        QString info = QString("recv failed: %1").arg(strerror(err));
        QMessageBox::warning(NULL, "Warning", info);
        return;
    }

    auto duration = chrono::steady_clock::now() - now;
    auto ms = chrono::duration_cast<chrono::milliseconds>(duration);
    auto us = chrono::duration_cast<chrono::microseconds>(duration);
    us = us - ms;
    char text_buf[sizeof(".000")] = {0};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-overflow"
    sprintf(text_buf, ".%03ld", us.count());
#pragma GCC diagnostic pop
    std::string text = std::to_string(ms.count()) + text_buf;

    // update ui
    ui->tickBtn->setText(QString::fromStdString(text + " ms"));

    static int c = 0;
    Qt::GlobalColor colors[2] = { Qt::green, Qt::red };
    QColor col = QColor(colors[c++ % 2]);
    QString qss = QString(rqss).arg(col.name());
    ui->tickBtn->setStyleSheet(qss);
}

void Dialog::slot2() {
    qDebug() << "reset";
    ui->tickBtn->setText("");
    ui->tickBtn->setStyleSheet(qss);
    ui->tickBtn->setEnabled(true);
}
