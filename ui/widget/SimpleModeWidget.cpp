#include "SimpleModeWidget.h"

#include "db/Database.hpp"
#include "main/NekoGui.hpp"
#include "ui/mainwindow_interface.h"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QResizeEvent>

SimpleModeWidget::SimpleModeWidget(QWidget *parent) : QWidget(parent) {
    setObjectName("SimpleModeWidget");

    advancedBtn = new QPushButton(tr("Advanced"), this);
    bgBtn = new QPushButton(tr("Theme"), this);
    advancedBtn->setCursor(Qt::PointingHandCursor);
    bgBtn->setCursor(Qt::PointingHandCursor);

    glass = new QWidget(this);
    glass->setObjectName("simpleGlass");

    statusLabel = new QLabel(tr("Disconnected"), glass);
    statusLabel->setAlignment(Qt::AlignCenter);

    serverCombo = new QComboBox(glass);
    serverCombo->setMinimumHeight(36);

    powerBtn = new QPushButton(tr("Connect"), glass);
    powerBtn->setCheckable(true);
    powerBtn->setMinimumHeight(52);
    powerBtn->setCursor(Qt::PointingHandCursor);
    powerBtn->setObjectName("powerBtn");

    rulesBtn = new QPushButton(tr("Rules"), glass);
    rulesBtn->setMinimumHeight(36);
    rulesBtn->setCursor(Qt::PointingHandCursor);

    auto *gl = new QVBoxLayout(glass);
    gl->setContentsMargins(18, 18, 18, 18);
    gl->setSpacing(12);
    gl->addWidget(statusLabel);
    gl->addWidget(serverCombo);
    gl->addWidget(powerBtn);
    gl->addWidget(rulesBtn);

    auto *shadow = new QGraphicsDropShadowEffect(glass);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0, 0, 0, 120));
    glass->setGraphicsEffect(shadow);

    setStyleSheet(
        "QPushButton#powerBtn {"
        "  background: rgba(40,120,255,210);"
        "  color: white;"
        "  border: none;"
        "  border-radius: 14px;"
        "  font-size: 18px;"
        "  font-weight: 700;"
        "}"
        "QPushButton#powerBtn:checked {"
        "  background: rgba(220,60,70,220);"
        "}"
        "QPushButton#powerBtn:hover { background: rgba(60,140,255,230); }"
        "QPushButton#powerBtn:checked:hover { background: rgba(235,80,90,230); }"
        "#simpleGlass {"
        "  background: rgba(18,20,28,155);"
        "  border-radius: 18px;"
        "  border: 1px solid rgba(255,255,255,40);"
        "}"
        "#simpleGlass QLabel { color: #f2f2f5; font-size: 13px; }"
        "#simpleGlass QComboBox {"
        "  background: rgba(255,255,255,230);"
        "  border-radius: 10px;"
        "  padding: 6px 10px;"
        "  border: none;"
        "}"
        "#simpleGlass QPushButton {"
        "  background: rgba(255,255,255,210);"
        "  border: none;"
        "  border-radius: 10px;"
        "  padding: 8px 12px;"
        "  font-weight: 600;"
        "}"
        "QPushButton {"
        "  background: rgba(20,22,30,160);"
        "  color: white;"
        "  border: 1px solid rgba(255,255,255,50);"
        "  border-radius: 10px;"
        "  padding: 6px 12px;"
        "}");

    connect(advancedBtn, &QPushButton::clicked, this, &SimpleModeWidget::requestAdvancedMode);
    connect(bgBtn, &QPushButton::clicked, this, &SimpleModeWidget::requestToggleBackground);
    connect(rulesBtn, &QPushButton::clicked, this, &SimpleModeWidget::requestEditRules);
    connect(powerBtn, &QPushButton::clicked, this, [=](bool checked) {
        int id = serverCombo->currentData().toInt();
        emit requestPowerToggle(checked, id);
    });
    connect(serverCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int) {
        auto id = serverCombo->currentData().toInt();
        if (id >= 0) {
            NekoGui::dataStore->simple_last_profile_id = id;
            NekoGui::dataStore->Save();
        }
    });

    reloadBackground();
    refreshServers();
    refreshPowerState();
}

void SimpleModeWidget::reloadBackground() {
    int ver = NekoGui::dataStore->ui_simple_bg;
    if (ver != 2) ver = 1;
    QString path = ver == 2 ? ":/neko/ver2.jpg" : ":/neko/ver1.jpg";
    bg = QPixmap(path);
    // fallback to file next to cwd / repo root
    if (bg.isNull()) {
        bg = QPixmap(ver == 2 ? "ver2.jpg" : "ver1.jpg");
    }
    update();
}

void SimpleModeWidget::refreshServers() {
    serverCombo->blockSignals(true);
    serverCombo->clear();
    auto group = NekoGui::profileManager->CurrentGroup();
    int selectId = NekoGui::dataStore->simple_last_profile_id;
    if (selectId < 0) selectId = NekoGui::dataStore->remember_id;
    if (selectId < 0) selectId = NekoGui::dataStore->started_id;
    int selectIndex = 0;
    if (group) {
        int i = 0;
        for (const auto &pf: group->ProfilesWithOrder()) {
            serverCombo->addItem(pf->bean->DisplayTypeAndName(), pf->id);
            if (pf->id == selectId) selectIndex = i;
            ++i;
        }
    }
    if (serverCombo->count() == 0) {
        serverCombo->addItem(tr("No servers"), -1);
    }
    serverCombo->setCurrentIndex(selectIndex);
    serverCombo->blockSignals(false);
}

void SimpleModeWidget::refreshPowerState() {
    bool on = NekoGui::dataStore->started_id >= 0;
    powerBtn->blockSignals(true);
    powerBtn->setChecked(on);
    powerBtn->setText(on ? tr("Disconnect") : tr("Connect"));
    powerBtn->blockSignals(false);
    if (on) {
        auto ent = NekoGui::profileManager->GetProfile(NekoGui::dataStore->started_id);
        QString name = ent ? ent->bean->DisplayName() : QString::number(NekoGui::dataStore->started_id);
        statusLabel->setText(tr("Connected · %1").arg(name));
        // sync combo
        for (int i = 0; i < serverCombo->count(); ++i) {
            if (serverCombo->itemData(i).toInt() == NekoGui::dataStore->started_id) {
                serverCombo->setCurrentIndex(i);
                break;
            }
        }
    } else {
        statusLabel->setText(NekoGui::dataStore->spmode_vpn ? tr("TUN on · Idle") : tr("Disconnected"));
    }
}

void SimpleModeWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    if (!bg.isNull()) {
        // cover crop
        QSize target = size();
        QSize src = bg.size();
        double scale = qMax((double) target.width() / src.width(), (double) target.height() / src.height());
        int w = (int) (src.width() * scale);
        int h = (int) (src.height() * scale);
        QRect dest((target.width() - w) / 2, (target.height() - h) / 2, w, h);
        p.drawPixmap(dest, bg);
    } else {
        p.fillRect(rect(), QColor(28, 30, 38));
    }
    // soft vignette at bottom for readability
    QLinearGradient g(0, height() * 0.45, 0, height());
    g.setColorAt(0, QColor(0, 0, 0, 0));
    g.setColorAt(1, QColor(0, 0, 0, 120));
    p.fillRect(rect(), g);
}

void SimpleModeWidget::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    rebuildChromeLayout();
}

void SimpleModeWidget::rebuildChromeLayout() {
    const int m = 14;
    advancedBtn->setParent(this);
    bgBtn->setParent(this);
    advancedBtn->adjustSize();
    bgBtn->adjustSize();
    advancedBtn->setFixedHeight(32);
    bgBtn->setFixedHeight(32);
    advancedBtn->move(m, m);
    bgBtn->move(width() - m - bgBtn->width(), m);
    advancedBtn->raise();
    bgBtn->raise();

    int glassW = qMin(width() - 28, 340);
    int glassH = 220;
    int gx = (width() - glassW) / 2;
    int gy = height() - glassH - 28;
    glass->setGeometry(gx, gy, glassW, glassH);
    glass->raise();
}
