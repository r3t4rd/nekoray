#pragma once

#include <QWidget>
#include "db/Group.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
    class DialogEditGroup;
}
QT_END_NAMESPACE

class DialogEditGroup : public QWidget {
    Q_OBJECT

public:
    explicit DialogEditGroup(const std::shared_ptr<NekoGui::Group> &ent, QWidget *parent = nullptr);

    ~DialogEditGroup() override;

signals:

    void pageFinished(bool saved);

private:
    Ui::DialogEditGroup *ui;

    std::shared_ptr<NekoGui::Group> ent;

    struct {
        int front_proxy;
    } CACHE;

    void refresh_front_proxy();

private slots:

    void apply();

    void onCancel();

    void on_front_proxy_clicked();
};
