#pragma once

#include <QWidget>
#include <QPixmap>

class QComboBox;
class QPushButton;
class QLabel;

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

private:
    void rebuildChromeLayout();

    QPixmap bg;
    QComboBox *serverCombo = nullptr;
    QPushButton *powerBtn = nullptr;
    QPushButton *rulesBtn = nullptr;
    QPushButton *advancedBtn = nullptr;
    QPushButton *bgBtn = nullptr;
    QLabel *statusLabel = nullptr;
    QWidget *glass = nullptr;
};
