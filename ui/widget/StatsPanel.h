#pragma once

#include <QWidget>
#include <QLabel>
#include <QVector>
#include <QColor>

class TrafficSparkline : public QWidget {
    Q_OBJECT
public:
    // Distinct palette shared by Advanced Stats + Simple Mode
    static QColor colorProxyUp() { return QColor(255, 122, 69); }   // coral
    static QColor colorProxyDown() { return QColor(76, 141, 255); } // blue
    static QColor colorDirectUp() { return QColor(240, 200, 74); }  // gold
    static QColor colorDirectDown() { return QColor(62, 207, 142); }// mint

    explicit TrafficSparkline(QWidget *parent = nullptr);
    void setSeries(const QVector<double> &up, const QVector<double> &down);
    void setColors(const QColor &up, const QColor &down);
    void setMultiSeries(const QVector<QVector<double>> &series, const QVector<QColor> &colors);
    void setTransparentBackground(bool on);
    void setShowWindowLabel(bool on);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QVector<double>> seriesList;
    QVector<QColor> colorList;
    QColor upColor = colorProxyUp();
    QColor downColor = colorProxyDown();
    bool transparentBg = false;
    bool showLabel = true;
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
