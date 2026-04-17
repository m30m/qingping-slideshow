#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>
#include <QSocketNotifier>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>

static int sigusr1_fds[2];
static int sigusr2_fds[2];

static void sigusr1_handler(int) {
    char c = 1;
    write(sigusr1_fds[1], &c, 1);
}

static void sigusr2_handler(int) {
    char c = 1;
    write(sigusr2_fds[1], &c, 1);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    if (argc < 2) {
        fprintf(stderr, "Usage: qtapp <image-url> [interval-seconds]\n");
        return 1;
    }
    QString url = argv[1];
    int interval = argc > 2 ? atoi(argv[2]) : 5;

    QWidget root;
    root.setStyleSheet("background-color: black;");

    auto* imageLabel = new QLabel(&root);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("background-color: black;");

    auto* status = new QLabel(&root);
    status->setAlignment(Qt::AlignCenter);
    status->setStyleSheet("color: #666; font-size: 18px;");
    status->setText("Loading...");

    auto* layout = new QVBoxLayout(&root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(imageLabel);
    layout->addWidget(status);
    layout->addStretch();

    auto* nam = new QNetworkAccessManager(&app);

    auto fetchImage = [=] {
        QNetworkRequest req{QUrl{url}};
        QNetworkReply* reply = nam->get(req);
        QObject::connect(reply, &QNetworkReply::finished, [=] {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                status->setText(reply->errorString());
                status->show();
                return;
            }
            QByteArray data = reply->readAll();
            QPixmap pix;
            if (pix.loadFromData(data)) {
                imageLabel->setPixmap(
                    pix.scaled(720, 720, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                status->hide();
            } else {
                status->setText("Failed to decode image");
                status->show();
            }
        });
    };
    fetchImage();

    QTimer slideTimer;
    QObject::connect(&slideTimer, &QTimer::timeout, fetchImage);
    slideTimer.start(interval * 1000);

    // SIGUSR1: bring our window to front
    socketpair(AF_UNIX, SOCK_STREAM, 0, sigusr1_fds);
    signal(SIGUSR1, sigusr1_handler);
    auto* n1 = new QSocketNotifier(sigusr1_fds[0], QSocketNotifier::Read, &app);
    QObject::connect(n1, &QSocketNotifier::activated, [&root] {
        char c;
        read(sigusr1_fds[0], &c, 1);
        root.hide();
        QTimer::singleShot(50, [&root] { root.showFullScreen(); });
    });

    // SIGUSR2: hide our window so QingSnow2App becomes visible
    socketpair(AF_UNIX, SOCK_STREAM, 0, sigusr2_fds);
    signal(SIGUSR2, sigusr2_handler);
    auto* n2 = new QSocketNotifier(sigusr2_fds[0], QSocketNotifier::Read, &app);
    QObject::connect(n2, &QSocketNotifier::activated, [&root] {
        char c;
        read(sigusr2_fds[0], &c, 1);
        root.hide();
    });

    root.resize(720, 720);
    root.show();
    return app.exec();
}
