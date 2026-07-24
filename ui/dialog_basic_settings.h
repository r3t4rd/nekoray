#ifndef DIALOG_BASIC_SETTINGS_H
#define DIALOG_BASIC_SETTINGS_H

#include <QWidget>
#include <QJsonObject>

namespace Ui {
    class DialogBasicSettings;
}

class DialogBasicSettings : public QWidget {
    Q_OBJECT

public:
    explicit DialogBasicSettings(QWidget *parent = nullptr);

    ~DialogBasicSettings();

    void setSection(int tabIndex);

public slots:

    void apply();

    void onCancel();

private:
    Ui::DialogBasicSettings *ui;

    struct {
        QJsonObject extraCore;
        QString custom_inbound;
        bool needRestart = false;
    } CACHE;

private slots:

    void refresh_auth();

    void on_set_custom_icon_clicked();

    void on_inbound_auth_clicked();

    void on_core_settings_clicked();
};

#endif // DIALOG_BASIC_SETTINGS_H
