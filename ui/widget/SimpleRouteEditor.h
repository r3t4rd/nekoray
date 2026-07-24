#pragma once

#include <QDialog>
#include <QStringList>

class QListWidget;
class QLineEdit;

// Friendly editor for custom_route_global:
//  - Direct domains (domain_suffix → direct)
//  - Proxy apps (process_name → proxy)
class SimpleRouteEditor : public QDialog {
    Q_OBJECT

public:
    explicit SimpleRouteEditor(QWidget *parent = nullptr);

    void loadFromJson(const QString &json);
    [[nodiscard]] QString toJson() const;

private slots:
    void addDomain();
    void removeDomain();
    void addProcess();
    void removeProcess();
    void onAccept();

private:
    QListWidget *domainList = nullptr;
    QListWidget *processList = nullptr;
    QLineEdit *domainEdit = nullptr;
    QLineEdit *processEdit = nullptr;
    QStringList otherRulesJson; // preserve unknown rules as raw JSON objects
};
