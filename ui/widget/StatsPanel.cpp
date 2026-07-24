#include "StatsPanel.h"

#include "main/NekoGui_Utils.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QPainterPath>
#include <QFrame>

TrafficSparkline::TrafficSparkline(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(72);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void TrafficSparkline::setSeries(const QVector<double> &up, const QVector<double> &down) {
    upSeries = up;
    downSeries = down;
    update();
}

void TrafficSparkline::setColors(const QColor &up, const QColor &down) {
    upColor = up;
    downColor = down;
}

void TrafficSparkline::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), QColor(30, 32, 38));

    auto drawSeries = [&](const QVector<double> &series, const QColor &color) {
        if (series.size() < 2) return;
        double maxV = 1.0;
        for (double v: series) maxV = qMax(maxV, v);
        QPainterPath path;
        const int n = series.size();
        for (int i = 0; i < n; ++i) {
            double x = (double) i / (n - 1) * (width() - 2) + 1;
            double y = height() - 2 - (series[i] / maxV) * (height() - 8);
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }
        QPainterPath fill = path;
        fill.lineTo(width() - 1, height() - 1);
        fill.lineTo(1, height() - 1);
        fill.closeSubpath();
        QColor fillC = color;
        fillC.setAlpha(60);
        p.fillPath(fill, fillC);
        QPen pen(color, 1.8);
        p.setPen(pen);
        p.drawPath(path);
    };

    drawSeries(downSeries, downColor);
    drawSeries(upSeries, upColor);

    p.setPen(QColor(180, 180, 180));
    p.drawText(8, 16, QStringLiteral("1 min"));
}

class ShareBarWidget : public QWidget {
public:
    double proxyRatio = 0.5;
    explicit ShareBarWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(18);
        setMaximumHeight(22);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QRectF r = rect().adjusted(1, 3, -1, -3);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(56, 132, 255));
        double w = r.width() * qBound(0.0, proxyRatio, 1.0);
        p.drawRoundedRect(QRectF(r.left(), r.top(), w, r.height()), 4, 4);
        p.setBrush(QColor(46, 160, 67));
        p.drawRoundedRect(QRectF(r.left() + w, r.top(), r.width() - w, r.height()), 4, 4);
    }
};

StatsPanel::StatsPanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto *speedRow = new QHBoxLayout;
    auto makeCard = [&](const QString &title, QLabel **speedOut, QLabel **totalOut, TrafficSparkline **chartOut, const QColor &up, const QColor &down) {
        auto *box = new QFrame(this);
        box->setFrameShape(QFrame::NoFrame);
        box->setStyleSheet("QFrame { background: #1e1f24; border-radius: 8px; }");
        auto *vl = new QVBoxLayout(box);
        auto *t = new QLabel(title, box);
        t->setStyleSheet("color:#aaa; font-size:12px;");
        *speedOut = new QLabel(QStringLiteral("↑ 0 B/s  ↓ 0 B/s"), box);
        (*speedOut)->setStyleSheet("color:#eee; font-size:16px; font-weight:600;");
        *totalOut = new QLabel(QStringLiteral("0 B↑  0 B↓"), box);
        (*totalOut)->setStyleSheet("color:#9ab; font-size:12px;");
        *chartOut = new TrafficSparkline(box);
        (*chartOut)->setColors(up, down);
        vl->addWidget(t);
        vl->addWidget(*speedOut);
        vl->addWidget(*totalOut);
        vl->addWidget(*chartOut, 1);
        return box;
    };

    speedRow->addWidget(makeCard(tr("Proxied"), &proxySpeed, &proxyTotal, &proxyChart, QColor(255, 140, 60), QColor(56, 132, 255)), 1);
    speedRow->addWidget(makeCard(tr("Direct"), &directSpeed, &directTotal, &directChart, QColor(255, 140, 60), QColor(46, 160, 67)), 1);
    layout->addLayout(speedRow, 1);

    auto *shareTitle = new QLabel(tr("Session volume share"), this);
    shareTitle->setStyleSheet("color:#aaa;");
    layout->addWidget(shareTitle);

    auto *shareRow = new QHBoxLayout;
    proxyShare = new QLabel(tr("Proxy 0%"), this);
    directShare = new QLabel(tr("Direct 0%"), this);
    auto *bar = new ShareBarWidget(this);
    shareBar = bar;
    shareRow->addWidget(proxyShare);
    shareRow->addWidget(bar, 1);
    shareRow->addWidget(directShare);
    layout->addLayout(shareRow);

    proxyUpHist.reserve(kWindow);
    proxyDownHist.reserve(kWindow);
    directUpHist.reserve(kWindow);
    directDownHist.reserve(kWindow);
}

void StatsPanel::resetSession() {
    proxyUpHist.clear();
    proxyDownHist.clear();
    directUpHist.clear();
    directDownHist.clear();
    lastProxyUp = lastProxyDown = lastDirectUp = lastDirectDown = 0;
    proxyChart->setSeries({}, {});
    directChart->setSeries({}, {});
    refreshShareBar();
}

void StatsPanel::pushSample(qint64 proxyUpRate, qint64 proxyDownRate,
                            qint64 directUpRate, qint64 directDownRate,
                            qint64 proxyUpTotal, qint64 proxyDownTotal,
                            qint64 directUpTotal, qint64 directDownTotal) {
    auto push = [](QVector<double> &v, double x) {
        v.append(x);
        while (v.size() > kWindow) v.removeFirst();
    };
    push(proxyUpHist, (double) proxyUpRate);
    push(proxyDownHist, (double) proxyDownRate);
    push(directUpHist, (double) directUpRate);
    push(directDownHist, (double) directDownRate);

    lastProxyUp = proxyUpTotal;
    lastProxyDown = proxyDownTotal;
    lastDirectUp = directUpTotal;
    lastDirectDown = directDownTotal;

    proxySpeed->setText(QStringLiteral("↑ %1  ↓ %2").arg(ReadableSize(proxyUpRate) + "/s", ReadableSize(proxyDownRate) + "/s"));
    directSpeed->setText(QStringLiteral("↑ %1  ↓ %2").arg(ReadableSize(directUpRate) + "/s", ReadableSize(directDownRate) + "/s"));
    proxyTotal->setText(QStringLiteral("%1↑  %2↓").arg(ReadableSize(proxyUpTotal), ReadableSize(proxyDownTotal)));
    directTotal->setText(QStringLiteral("%1↑  %2↓").arg(ReadableSize(directUpTotal), ReadableSize(directDownTotal)));

    proxyChart->setSeries(proxyUpHist, proxyDownHist);
    directChart->setSeries(directUpHist, directDownHist);
    refreshShareBar();
}

void StatsPanel::refreshShareBar() {
    double p = (double) (lastProxyUp + lastProxyDown);
    double d = (double) (lastDirectUp + lastDirectDown);
    double sum = p + d;
    double ratio = sum > 0 ? p / sum : 0.5;
    if (shareBar) {
        shareBar->proxyRatio = ratio;
        shareBar->update();
    }
    int pp = (int) qRound(ratio * 100);
    proxyShare->setText(tr("Proxy %1%").arg(pp));
    directShare->setText(tr("Direct %1%").arg(100 - pp));
}
