#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
    class DialogVPNSettings;
}
QT_END_NAMESPACE

class DialogVPNSettings : public QWidget {
    Q_OBJECT

public:
    explicit DialogVPNSettings(QWidget *parent = nullptr);

    ~DialogVPNSettings() override;

public slots:

    void apply();

    void onCancel();

private:
    Ui::DialogVPNSettings *ui;

private slots:

    void on_troubleshooting_clicked();
};
