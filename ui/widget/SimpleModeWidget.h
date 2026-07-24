#pragma once

#include <QWidget>
#include <QPixmap>
#include <QList>

class QPushButton;
class QLabel;
class QScrollArea;
class QVBoxLayout;
class QFrame;

class SimpleModeWidget : public QWidget {
    Q_OBJECT

public:
    explicit SimpleModeWidget(QWidget *parent = nullptr);

    void refreshServers();
    void refreshPowerState();
    void reloadBackground();

signals:
    void requestAdvancedMode();
    void requestToggleBackground();
    void requestPowerToggle(bool turnOn, int profileId);
    void requestEditRules();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void rebuildChromeLayout();
    QRect mapDesignRect(int x, int y, int w, int h) const;
    void updateHeroText();
    int selectedProfileId() const;
    void applyChromeButtonStyle(QPushButton *b);
    void applyServerCardStyle(QPushButton *card, bool checked);

    static constexpr int kDesignW = 824;
    static constexpr int kDesignH = 1280;
    static constexpr int kHeroX = 122;
    static constexpr int kHeroY = 470;
    static constexpr int kHeroW = 584;
    static constexpr int kHeroH = 115;

    QPixmap bg;
    QPushButton *advancedBtn = nullptr;
    QPushButton *bgBtn = nullptr;
    QPushButton *rulesBtn = nullptr;

    QFrame *hero = nullptr;
    QLabel *heroTitle = nullptr;
    QLabel *heroSub = nullptr; // status under the hero block

    QScrollArea *serverScroll = nullptr;
    QWidget *serverListHost = nullptr;
    QVBoxLayout *serverListLayout = nullptr;
    QList<QPushButton *> serverCards;
    int selectedId = -1;
};
