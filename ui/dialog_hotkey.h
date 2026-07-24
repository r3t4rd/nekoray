#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
    class DialogHotkey;
}
QT_END_NAMESPACE

class DialogHotkey : public QWidget {
    Q_OBJECT

public:
    explicit DialogHotkey(QWidget *parent = nullptr);

    ~DialogHotkey() override;

public slots:

    void apply();

    void onCancel();

private:
    Ui::DialogHotkey *ui;
};
