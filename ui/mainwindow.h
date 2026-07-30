#pragma once

#include <QMainWindow>

#include "main/NekoGui.hpp"

#include <memory>

namespace NekoGui {
    class Group;
}

#ifndef MW_INTERFACE

#include <QTime>
#include <QTableWidgetItem>
#include <QKeyEvent>
#include <QSystemTrayIcon>
#include <QProcess>
#include <QTextDocument>
#include <QShortcut>
#include <QSemaphore>
#include <QMutex>
#include <atomic>

#include "GroupSort.hpp"

#include "db/ProxyEntity.hpp"
#include "db/Group.hpp"
#include "main/GuiUtils.hpp"

class DialogBasicSettings;
class DialogManageGroups;
class DialogManageRoutes;
class DialogVPNSettings;
class DialogHotkey;
class DetailsPanel;
class StatsPanel;
class SimpleModeWidget;

#endif

namespace NekoGui_sys {
    class CoreProcess;
}

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() override;

    void refresh_proxy_list(const int &id = -1);

    void show_group(int gid);

    void refresh_groups();

    void refresh_status(const QString &traffic_update = "");

    void neko_start(int _id = -1);

    void neko_stop(bool crash = false, bool sem = false);

    void neko_set_spmode_system_proxy(bool enable, bool save = true);

    void neko_set_spmode_vpn(bool enable, bool save = true);

    void show_log_impl(const QString &log);

    void start_select_mode(QObject *context, const std::function<void(int)> &callback);

    void refresh_connection_list(const QJsonArray &arr);

    void updateTrafficCharts(qint64 proxyUpRate, qint64 proxyDownRate,
                             qint64 directUpRate, qint64 directDownRate,
                             qint64 proxyUpTotal, qint64 proxyDownTotal,
                             qint64 directUpTotal, qint64 directDownTotal);

    void applyUiMode(bool simple);

    void showSimpleMode();

    void showAdvancedMode();

    void RegisterHotkey(bool unregister);

    bool StopVPNProcess(bool unconditional = false);

    void navigateTo(const QString &pageId, int section = -1);

    void openEditProfile(const QString &type, int id);

    void openEditGroup(const std::shared_ptr<NekoGui::Group> &ent);

signals:

    void profile_selected(int id);

public slots:

    void on_commitDataRequest();

    void on_menu_exit_triggered();

#ifndef MW_INTERFACE

private slots:

    void on_masterLogBrowser_customContextMenuRequested(const QPoint &pos);

    void on_menu_basic_settings_triggered();

    void on_menu_routing_settings_triggered();

    void on_menu_vpn_settings_triggered();

    void on_menu_hotkey_settings_triggered();

    void on_menu_add_from_input_triggered();

    void on_menu_add_from_clipboard_triggered();

    void on_menu_clone_triggered();

    void on_menu_move_triggered();

    void on_menu_delete_triggered();

    void on_menu_reset_traffic_triggered();

    void on_menu_profile_debug_info_triggered();

    void on_menu_copy_links_triggered();

    void on_menu_copy_links_nkr_triggered();

    void on_menu_export_config_triggered();

    void display_qr_link(bool nkrFormat = false);

    void on_menu_scan_qr_triggered();

    void on_menu_clear_test_result_triggered();

    void on_menu_manage_groups_triggered();

    void on_menu_select_all_triggered();

    void on_menu_delete_repeat_triggered();

    void on_menu_remove_unavailable_triggered();

    void on_menu_update_subscription_triggered();

    void on_menu_resolve_domain_triggered();

    void on_proxyListTable_itemDoubleClicked(QTableWidgetItem *item);

    void on_proxyListTable_customContextMenuRequested(const QPoint &pos);

    void on_tabWidget_currentChanged(int index);

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *tray;
    QShortcut *shortcut_ctrl_f = new QShortcut(QKeySequence("Ctrl+F"), this);
    QShortcut *shortcut_esc = new QShortcut(QKeySequence("Esc"), this);
    //
    NekoGui_sys::CoreProcess *core_process;
    qint64 vpn_pid = 0;
    //
    bool qvLogAutoScoll = true;
    QTextDocument *qvLogDocument = new QTextDocument(this);
    //
    QString title_error;
    int icon_status = -1;
    std::shared_ptr<NekoGui::ProxyEntity> running;
    QString traffic_update_cache;
    QTime last_test_time;
    //
    int proxy_last_order = -1;
    bool select_mode = false;
    QMutex mu_starting;
    QMutex mu_stopping;
    QMutex mu_exit;
    QSemaphore sem_stopped;
    std::atomic_bool mu_starting_held{false};
    std::atomic_bool mu_stopping_held{false};
    std::atomic_bool action_cancelled{false};
    int exit_reason = 0;

    void cancelLastAction();

    bool tryLockStarting();

    void unlockStarting();

    bool tryLockStopping();

    void unlockStopping();

    void forceUnlockActions();

    QList<std::shared_ptr<NekoGui::ProxyEntity>> get_now_selected_list();

    QList<std::shared_ptr<NekoGui::ProxyEntity>> get_selected_or_group();

    void dialog_message_impl(const QString &sender, const QString &info);

    void refresh_proxy_list_impl(const int &id = -1, GroupSortAction groupSortAction = {});

    void refresh_proxy_list_impl_refresh_data(const int &id = -1);

    void keyPressEvent(QKeyEvent *event) override;

    void closeEvent(QCloseEvent *event) override;

    void setupNavigation();

    class QWidget *navColumn = nullptr;
    class QTreeWidget *navTree = nullptr;
    class QStackedWidget *pageStack = nullptr;
    QWidget *pageProxies = nullptr;
    DialogBasicSettings *pageBasic = nullptr;
    DialogManageGroups *pageGroups = nullptr;
    DialogManageRoutes *pageRoutes = nullptr;
    DialogVPNSettings *pageVpn = nullptr;
    DialogHotkey *pageHotkey = nullptr;
    QWidget *pageEditProfileHost = nullptr;
    QWidget *pageEditGroupHost = nullptr;
    QString navReturnPage = "proxies";
    DetailsPanel *detailsPanel = nullptr;
    StatsPanel *statsPanel = nullptr;
    SimpleModeWidget *simpleMode = nullptr;
    QSize advancedSizeHint{1000, 700};

    //

    void HotkeyEvent(const QString &key);

    bool StartVPNProcess();

    // grpc and ...

    static void setup_grpc();

    void speedtest_current_group(int mode, bool test_group);

    void speedtest_current();

    static void stop_core_daemon();

    void CheckUpdate(bool silentIfNoUpdate = false);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

#endif // MW_INTERFACE
};

inline MainWindow *GetMainWindow() {
    return (MainWindow *) mainwindow;
}

void UI_InitMainWindow();
