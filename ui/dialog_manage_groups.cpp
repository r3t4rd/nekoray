#include "dialog_manage_groups.h"
#include "ui_dialog_manage_groups.h"

#include "db/Database.hpp"
#include "sub/GroupUpdater.hpp"
#include "main/GuiUtils.hpp"
#include "ui/widget/GroupItem.h"
#include "ui/mainwindow_interface.h"

#include <QListWidgetItem>
#include <QMessageBox>

#define AddGroupToListIfExist(_id)                       \
    auto __ent = NekoGui::profileManager->GetGroup(_id); \
    if (__ent != nullptr) {                              \
        auto wI = new QListWidgetItem();                 \
        auto w = new GroupItem(this, __ent, wI);         \
        wI->setData(114514, _id);                        \
        ui->listWidget->addItem(wI);                     \
        ui->listWidget->setItemWidget(wI, w);            \
    }

DialogManageGroups::DialogManageGroups(QWidget *parent) : QWidget(parent), ui(new Ui::DialogManageGroups) {
    ui->setupUi(this);
    refreshData();

    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, [=](QListWidgetItem *wI) {
        auto id = wI->data(114514).toInt();
        emit requestEditGroup(id);
    });
}

DialogManageGroups::~DialogManageGroups() {
    delete ui;
}

void DialogManageGroups::refreshData() {
    ui->listWidget->clear();
    for (auto id: NekoGui::profileManager->groupsTabOrder) {
        AddGroupToListIfExist(id)
    }
}

void DialogManageGroups::on_add_clicked() {
    emit requestAddGroup();
}

void DialogManageGroups::on_update_all_clicked() {
    if (QMessageBox::question(this, tr("Confirmation"), tr("Update all subscriptions?")) == QMessageBox::StandardButton::Yes) {
        UI_update_all_groups();
    }
}
