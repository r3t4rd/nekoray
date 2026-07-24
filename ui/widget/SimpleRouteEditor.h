#pragma once

#include <QDialog>
#include <QStringList>
#include <QJsonArray>

class QListWidget;
class QLineEdit;
class QPlainTextEdit;
class QTabWidget;

// Friendly editor for Custom Route JSON (routing->custom + custom_route_global).
class SimpleRouteEditor : public QDialog {
    Q_OBJECT

public:
    explicit SimpleRouteEditor(QWidget *parent = nullptr);

    void loadFromJson(const QString &json);
    [[nodiscard]] QString toJson() const;

private:
    struct ListPage {
        QListWidget *list = nullptr;
        QLineEdit *edit = nullptr;
    };

    ListPage makeDomainPage(QWidget *parent, const QString &placeholder);
    ListPage makeAppPage(QWidget *parent, const QString &placeholder);

    static void addListRow(QListWidget *list, const QString &text);
    static void removeSelected(QListWidget *list);
    static QJsonArray collectList(QListWidget *list);

    void syncJsonFromLists();
    bool syncListsFromJson();
    void parseRulesIntoLists(const QJsonArray &rules);

    QTabWidget *tabs = nullptr;
    ListPage directSites;
    ListPage proxySites;
    ListPage directApps;
    ListPage proxyApps;
    QPlainTextEdit *jsonEdit = nullptr;
    QStringList otherRulesJson;
    int lastTabIndex = 0;
    bool syncing = false;
};
