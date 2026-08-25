/* 应用入口只负责 Qt 生命周期、主题资源和主窗口创建。 */
#include "HomeWidge.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QScreen>
#include <QStyleFactory>

namespace {
void applyApplicationTheme(QApplication &application) {
    application.setStyle(QStyleFactory::create("Fusion"));
    application.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9));

    QFile themeFile(QStringLiteral(":/styles/theme.qss"));
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        application.setStyleSheet(QString::fromUtf8(themeFile.readAll()));
    }
}
}

int main(int argc, char *argv[]) {
    // 应用入口保持极薄，主窗口自己完成模块装配。
    QApplication a(argc, argv);
    applyApplicationTheme(a);
    HomeWidge w;

    const QRect available = w.screen()->availableGeometry();
    const QSize targetSize(qMin(1120, static_cast<int>(available.width() * 0.88)),
                           qMin(780, static_cast<int>(available.height() * 0.88)));
    w.resize(targetSize.expandedTo(w.minimumSize()));
    w.move(available.center() - w.rect().center());
    w.show();
    return a.exec();
}
