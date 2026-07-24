#pragma once

#include <QWidget>
#include <QLabel>
#include <QVector>
#include <QElapsedTimer>

class TrafficSparkline : public QWidget {
    Q_OBJECT
public:
    explicit TrafficSparkline(QWidget *parent = nullptr);
    void setSeries(const QVector<double> &up, const QVector<double> &down);
    void setColors(const QColor &up, const QColor &down);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> upSeries;
    QVector<double> downSeries;
    QColor upColor = QColor(46, 160, 67);
    QColor downColor = QColor(56, 132, 255);
};

class ShareBarWidget;

class StatsPanel : public QWidget {
    Q_OBJECT

public:
    explicit StatsPanel(QWidget *parent = nullptr);

    // rates in bytes/sec, totals in bytes
    void pushSample(qint64 proxyUpRate, qint64 proxyDownRate,
                    qint64 directUpRate, qint64 directDownRate,
                    qint64 proxyUpTotal, qint64 proxyDownTotal,
                    qint64 directUpTotal, qint64 directDownTotal);

    void resetSession();

private:
    static constexpr int kWindow = 60;

    QLabel *proxySpeed = nullptr;
    QLabel *directSpeed = nullptr;
    QLabel *proxyTotal = nullptr;
    QLabel *directTotal = nullptr;
    QLabel *proxyShare = nullptr;
    QLabel *directShare = nullptr;
    class ShareBarWidget *shareBar = nullptr;
    TrafficSparkline *proxyChart = nullptr;
    TrafficSparkline *directChart = nullptr;

    QVector<double> proxyUpHist;
    QVector<double> proxyDownHist;
    QVector<double> directUpHist;
    QVector<double> directDownHist;

    qint64 lastProxyUp = 0;
    qint64 lastProxyDown = 0;
    qint64 lastDirectUp = 0;
    qint64 lastDirectDown = 0;

    void refreshShareBar();
};
