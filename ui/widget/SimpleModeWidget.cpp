#include "SimpleModeWidget.h"

#include "db/Database.hpp"
#include "main/NekoGui.hpp"

#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QResizeEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QPalette>
#include <QSizePolicy>

static void makeTransparent(QWidget *w) {
    if (!w) return;
    w->setAttribute(Qt::WA_TranslucentBackground, true);
    w->setAutoFillBackground(false);
    QPalette pal = w->palette();
    pal.setColor(QPalette::Window, Qt::transparent);
    pal.setColor(QPalette::Base, Qt::transparent);
    w->setPalette(pal);
}

SimpleModeWidget::SimpleModeWidget(QWidget *parent) : QWidget(parent) {
    setObjectName("SimpleModeWidget");
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    advancedBtn = new QPushButton(tr("Advanced"), this);
    bgBtn = new QPushButton(tr("Theme"), this);
    rulesBtn = new QPushButton(tr("Rules"), this);
    for (auto *b: {advancedBtn, bgBtn, rulesBtn}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedHeight(28);
        applyChromeButtonStyle(b);
    }

    hero = new QFrame(this);
    hero->setObjectName("heroBlock");
    hero->setCursor(Qt::PointingHandCursor);
    hero->installEventFilter(this);
    hero->setStyleSheet(
        "#heroBlock {"
        "  background: rgba(10,12,18,150);"
        "  border-radius: 18px;"
        "  border: 1px solid rgba(255,255,255,35);"
        "}");
    auto *heroLay = new QVBoxLayout(hero);
    heroLay->setContentsMargins(16, 12, 16, 12);
    heroLay->setSpacing(0);
    heroTitle = new QLabel(tr("Подключись"), hero);
    heroTitle->setAlignment(Qt::AlignCenter);
    heroTitle->setStyleSheet(
        "color: #ffffff;"
        "font-size: 22px;"
        "font-weight: 700;"
        "letter-spacing: 1px;"
        "background: transparent;");
    heroLay->addStretch(1);
    heroLay->addWidget(heroTitle);
    heroLay->addStretch(1);

    // Status lives under the hero block (not inside it)
    heroSub = new QLabel(tr("Not connected"), this);
    heroSub->setAlignment(Qt::AlignCenter);
    heroSub->setStyleSheet("color: #e8a4a4; font-size: 13px; font-weight: 600; background: transparent;");

    serverScroll = new QScrollArea(this);
    serverScroll->setObjectName("serverScroll");
    serverScroll->setWidgetResizable(true);
    serverScroll->setFrameShape(QFrame::NoFrame);
    serverScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    serverScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    makeTransparent(serverScroll);
    makeTransparent(serverScroll->viewport());
    serverScroll->setStyleSheet(
        "QScrollArea#serverScroll { background: transparent; border: none; }"
        "QScrollArea#serverScroll > QWidget { background: transparent; }"
        "QScrollBar:vertical {"
        "  width: 5px; background: transparent; margin: 2px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: rgba(255,255,255,70); border-radius: 2px; min-height: 24px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }");

    serverListHost = new QWidget;
    serverListHost->setObjectName("serverListHost");
    makeTransparent(serverListHost);
    serverListHost->setStyleSheet("#serverListHost { background: transparent; }");
    serverListHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    serverListLayout = new QVBoxLayout(serverListHost);
    serverListLayout->setContentsMargins(12, 4, 12, 8);
    serverListLayout->setSpacing(5);
    serverListLayout->setAlignment(Qt::AlignTop);
    serverScroll->setWidget(serverListHost);

    connect(advancedBtn, &QPushButton::clicked, this, &SimpleModeWidget::requestAdvancedMode);
    connect(bgBtn, &QPushButton::clicked, this, &SimpleModeWidget::requestToggleBackground);
    connect(rulesBtn, &QPushButton::clicked, this, &SimpleModeWidget::requestEditRules);

    reloadBackground();
    refreshServers();
    refreshPowerState();
}

void SimpleModeWidget::applyChromeButtonStyle(QPushButton *b) {
    b->setStyleSheet(
        "QPushButton {"
        "  background: rgba(12,14,20,170);"
        "  color: #f0f0f3;"
        "  border: 1px solid rgba(255,255,255,40);"
        "  border-radius: 9px;"
        "  padding: 3px 10px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover { background: rgba(22,26,36,200); }");
}

void SimpleModeWidget::applyServerCardStyle(QPushButton *card, bool checked) {
    Q_UNUSED(checked)
    card->setStyleSheet(
        "QPushButton {"
        "  background: rgba(10,12,18,155);"
        "  color: #f0f0f3;"
        "  border: 1px solid rgba(255,255,255,38);"
        "  border-radius: 10px;"
        "  padding: 4px 12px;"
        "  text-align: left;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:checked {"
        "  background: rgba(18,28,48,200);"
        "  border: 1px solid rgba(140,170,220,140);"
        "}"
        "QPushButton:hover {"
        "  background: rgba(18,22,32,190);"
        "}");
}

QRect SimpleModeWidget::mapDesignRect(int x, int y, int w, int h) const {
    if (width() <= 0 || height() <= 0) return {};
    double scale = qMax((double) width() / kDesignW, (double) height() / kDesignH);
    double drawW = kDesignW * scale;
    double drawH = kDesignH * scale;
    double ox = (width() - drawW) / 2.0;
    double oy = (height() - drawH) / 2.0;
    return QRect(qRound(ox + x * scale), qRound(oy + y * scale), qRound(w * scale), qRound(h * scale));
}

void SimpleModeWidget::reloadBackground() {
    int ver = NekoGui::dataStore->ui_simple_bg;
    if (ver != 2) ver = 1;
    QString path = ver == 2 ? ":/neko/ver2.jpg" : ":/neko/ver1.jpg";
    bg = QPixmap(path);
    if (bg.isNull()) bg = QPixmap(ver == 2 ? "ver2.jpg" : "ver1.jpg");
    update();
}

int SimpleModeWidget::selectedProfileId() const {
    return selectedId;
}

void SimpleModeWidget::refreshServers() {
    // Fully clear layout — no leftover stretch / ghost widgets
    while (serverListLayout->count() > 0) {
        auto *item = serverListLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    serverCards.clear();

    if (selectedId < 0) {
        selectedId = NekoGui::dataStore->simple_last_profile_id;
        if (selectedId < 0) selectedId = NekoGui::dataStore->remember_id;
        if (selectedId < 0) selectedId = NekoGui::dataStore->started_id;
    }

    auto group = NekoGui::profileManager->CurrentGroup();
    if (!group) {
        serverListHost->adjustSize();
        return;
    }

    for (const auto &pf: group->ProfilesWithOrder()) {
        auto *card = new QPushButton(pf->bean->DisplayName(), serverListHost);
        card->setProperty("profileId", pf->id);
        card->setCheckable(true);
        card->setChecked(pf->id == selectedId);
        card->setFixedHeight(34);
        card->setCursor(Qt::PointingHandCursor);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        applyServerCardStyle(card, card->isChecked());

        connect(card, &QPushButton::clicked, this, [=] {
            selectedId = pf->id;
            NekoGui::dataStore->simple_last_profile_id = pf->id;
            NekoGui::dataStore->Save();
            for (auto *c: serverCards) {
                bool on = c->property("profileId").toInt() == selectedId;
                c->setChecked(on);
            }
            emit requestPowerToggle(true, selectedId);
        });

        serverListLayout->addWidget(card);
        serverCards << card;
    }

    serverListHost->adjustSize();
}

void SimpleModeWidget::updateHeroText() {
    bool on = NekoGui::dataStore->started_id >= 0;
    if (on) {
        auto ent = NekoGui::profileManager->GetProfile(NekoGui::dataStore->started_id);
        QString name = ent ? ent->bean->DisplayName() : QString::number(NekoGui::dataStore->started_id);
        heroTitle->setText(name);
        heroSub->setText(tr("Connected"));
        heroSub->setStyleSheet("color: #8fceb0; font-size: 13px; font-weight: 600; background: transparent;");
    } else {
        heroTitle->setText(tr("Подключись"));
        heroSub->setText(tr("Not connected"));
        heroSub->setStyleSheet("color: #e8a4a4; font-size: 13px; font-weight: 600; background: transparent;");
    }
}

void SimpleModeWidget::refreshPowerState() {
    bool on = NekoGui::dataStore->started_id >= 0;
    if (on) {
        selectedId = NekoGui::dataStore->started_id;
        for (auto *c: serverCards) {
            c->setChecked(c->property("profileId").toInt() == selectedId);
        }
    }
    updateHeroText();
}

void SimpleModeWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    if (!bg.isNull()) {
        double scale = qMax((double) width() / kDesignW, (double) height() / kDesignH);
        int w = (int) (kDesignW * scale);
        int h = (int) (kDesignH * scale);
        QRect dest((width() - w) / 2, (height() - h) / 2, w, h);
        p.drawPixmap(dest, bg);
    } else {
        p.fillRect(rect(), QColor(28, 30, 38));
    }
}

void SimpleModeWidget::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    rebuildChromeLayout();
}

bool SimpleModeWidget::eventFilter(QObject *obj, QEvent *event) {
    if (obj == hero && event->type() == QEvent::MouseButtonRelease) {
        bool on = NekoGui::dataStore->started_id >= 0;
        int id = selectedId;
        if (id < 0 && !serverCards.isEmpty()) {
            id = serverCards.first()->property("profileId").toInt();
        }
        emit requestPowerToggle(!on, id);
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void SimpleModeWidget::rebuildChromeLayout() {
    const int m = 12;
    advancedBtn->adjustSize();
    rulesBtn->adjustSize();
    bgBtn->adjustSize();
    advancedBtn->move(m, m);
    bgBtn->move(width() - m - bgBtn->width(), m);
    rulesBtn->move(bgBtn->x() - 8 - rulesBtn->width(), m);
    advancedBtn->raise();
    rulesBtn->raise();
    bgBtn->raise();

    QRect heroR = mapDesignRect(kHeroX, kHeroY, kHeroW, kHeroH);
    heroR = heroR.intersected(rect().adjusted(8, 40, -8, -8));
    if (heroR.height() < 56) heroR.setHeight(56);
    hero->setGeometry(heroR);
    hero->raise();

    heroSub->adjustSize();
    const int subW = qMin(width() - 24, qMax(heroR.width(), heroSub->sizeHint().width() + 20));
    const int subX = (width() - subW) / 2;
    const int subY = heroR.bottom() + 8;
    heroSub->setGeometry(subX, subY, subW, heroSub->sizeHint().height());
    heroSub->raise();

    // Full-width list, max 30% of window, pinned to bottom — no phantom stretch
    const int listMaxH = qMax(72, (int) (height() * 0.30));
    const int gap = 10;
    int listBottom = height() - 10;
    int listTop = listBottom - listMaxH;
    const int minTop = heroSub->geometry().bottom() + gap;
    if (listTop < minTop) listTop = minTop;
    int listH = listBottom - listTop;
    if (listH > 40) {
        serverScroll->setGeometry(0, listTop, width(), listH);
        serverScroll->show();
        serverScroll->raise();
        makeTransparent(serverScroll->viewport());
    } else {
        serverScroll->hide();
    }
}
