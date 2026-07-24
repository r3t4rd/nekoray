#include "DetailsPanel.h"

#include "main/NekoGui_Utils.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QFileInfo>
#include <QMap>
#include <QJsonObject>
#include <algorithm>

namespace {

struct AggNode {
    QString key;
    QString title;
    qint64 upload = 0;
    qint64 download = 0;
    qint64 lastSeen = 0;
    int active = 0;
    int total = 0;
    QString tag;
    QMap<QString, AggNode> children;
};

QString fmtTraffic(qint64 up, qint64 down) {
    return QStringLiteral("%1↑ %2↓").arg(ReadableSize(up), ReadableSize(down));
}

QString fmtLast(qint64 ts) {
    if (ts <= 0) return QObject::tr("n/a");
    return QDateTime::fromSecsSinceEpoch(ts).toString("HH:mm:ss");
}

void addConn(AggNode &root, const QStringList &path, const QJsonObject &obj) {
    AggNode *cur = &root;
    for (int i = 0; i < path.size(); ++i) {
        const QString &k = path[i];
        if (!cur->children.contains(k)) {
            AggNode n;
            n.key = k;
            n.title = k;
            cur->children.insert(k, n);
        }
        cur = &cur->children[k];
        cur->upload += obj["Upload"].toVariant().toLongLong();
        cur->download += obj["Download"].toVariant().toLongLong();
        qint64 start = obj["Start"].toVariant().toLongLong();
        qint64 end = obj["End"].toVariant().toLongLong();
        qint64 last = end > 0 ? end : start;
        if (last > cur->lastSeen) cur->lastSeen = last;
        cur->total += 1;
        if (obj["Active"].toBool()) cur->active += 1;
        if (i == path.size() - 1) {
            cur->tag = obj["Tag"].toString();
        }
    }
}

QList<AggNode *> sortedChildren(AggNode &node, int sortMode) {
    QList<AggNode *> list;
    for (auto it = node.children.begin(); it != node.children.end(); ++it) {
        list << &it.value();
    }
    std::sort(list.begin(), list.end(), [sortMode](AggNode *a, AggNode *b) {
        if (sortMode == 1) { // last
            if (a->lastSeen != b->lastSeen) return a->lastSeen > b->lastSeen;
        } else if (sortMode == 2) { // traffic
            auto ta = a->upload + a->download;
            auto tb = b->upload + b->download;
            if (ta != tb) return ta > tb;
        }
        return QString::compare(a->title, b->title, Qt::CaseInsensitive) < 0;
    });
    return list;
}

void fillTree(QTreeWidget *tree, QTreeWidgetItem *parent, AggNode &node, int sortMode, int depth) {
    auto kids = sortedChildren(node, sortMode);
    for (AggNode *c: kids) {
        auto *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree);
        QString prefix;
        if (depth == 0) prefix = "";
        else if (depth == 1) prefix = QObject::tr("FQDN: ");
        else prefix = QObject::tr("IP: ");

        QString status;
        if (c->active > 0) status = QObject::tr("%1 active").arg(c->active);
        else status = QObject::tr("idle");

        item->setText(0, prefix + c->title);
        item->setText(1, c->tag);
        item->setText(2, status);
        item->setText(3, fmtLast(c->lastSeen));
        item->setText(4, fmtTraffic(c->upload, c->download));
        item->setToolTip(0, c->title);
        item->setExpanded(depth < 1);
        fillTree(tree, item, *c, sortMode, depth + 1);
    }
}

bool nodeMatches(const AggNode &node, const QString &q) {
    if (q.isEmpty()) return true;
    if (node.title.contains(q, Qt::CaseInsensitive)) return true;
    for (auto it = node.children.begin(); it != node.children.end(); ++it) {
        if (nodeMatches(it.value(), q)) return true;
    }
    return false;
}

void filterNode(AggNode &node, const QString &q) {
    if (q.isEmpty()) return;
    QStringList remove;
    for (auto it = node.children.begin(); it != node.children.end(); ++it) {
        if (!nodeMatches(it.value(), q)) remove << it.key();
        else filterNode(it.value(), q);
    }
    for (const auto &k: remove) node.children.remove(k);
}

} // namespace

DetailsPanel::DetailsPanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto *bar = new QHBoxLayout;
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText(tr("Search process / FQDN / IP…"));
    searchEdit->setClearButtonEnabled(true);

    groupCombo = new QComboBox(this);
    groupCombo->addItem(tr("Process → FQDN → IP"), ByProcessHostIp);
    groupCombo->addItem(tr("FQDN → IP"), ByHostIp);
    groupCombo->addItem(tr("Process → IP"), ByProcessIp);

    sortCombo = new QComboBox(this);
    sortCombo->addItem(tr("Sort: Name"), SortByName);
    sortCombo->addItem(tr("Sort: Last seen"), SortByLast);
    sortCombo->addItem(tr("Sort: Traffic"), SortByTraffic);

    activeOnly = new QCheckBox(tr("Active only"), this);

    bar->addWidget(searchEdit, 1);
    bar->addWidget(groupCombo);
    bar->addWidget(sortCombo);
    bar->addWidget(activeOnly);
    layout->addLayout(bar);

    tree = new QTreeWidget(this);
    tree->setHeaderLabels({tr("Name"), tr("Outbound"), tr("Status"), tr("Last"), tr("Traffic")});
    tree->setUniformRowHeights(true);
    tree->setAlternatingRowColors(true);
    tree->setRootIsDecorated(true);
    tree->setAnimated(true);
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    layout->addWidget(tree, 1);

    summary = new QLabel(this);
    layout->addWidget(summary);

    connect(searchEdit, &QLineEdit::textChanged, this, &DetailsPanel::rebuildTree);
    connect(groupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DetailsPanel::rebuildTree);
    connect(sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DetailsPanel::rebuildTree);
    connect(activeOnly, &QCheckBox::toggled, this, &DetailsPanel::rebuildTree);
}

void DetailsPanel::updateConnections(const QJsonArray &arr) {
    lastConnections = arr;
    rebuildTree();
}

void DetailsPanel::rebuildTree() {
    tree->clear();
    AggNode root;
    root.title = "root";

    auto mode = (GroupMode) groupCombo->currentData().toInt();
    auto sortMode = sortCombo->currentData().toInt();
    auto onlyActive = activeOnly->isChecked();
    auto query = searchEdit->text().trimmed();

    int connCount = 0;
    int activeCount = 0;
    qint64 totalUp = 0, totalDown = 0;

    for (const auto &v: lastConnections) {
        auto obj = v.toObject();
        bool active = obj["Active"].toBool();
        if (onlyActive && !active) continue;

        QString process = obj["Process"].toString();
        if (process.isEmpty()) process = "(unknown)";
        QString host = obj["Host"].toString();
        if (host.isEmpty()) host = obj["Dest"].toString();
        QString ip = obj["DestIP"].toString();
        if (ip.isEmpty()) ip = host;
        if (obj["DestPort"].toInt() > 0) {
            ip = QStringLiteral("%1:%2").arg(ip).arg(obj["DestPort"].toInt());
        }

        QStringList path;
        switch (mode) {
            case ByHostIp:
                path = {host, ip};
                break;
            case ByProcessIp:
                path = {process, ip};
                break;
            case ByProcessHostIp:
            default:
                path = {process, host, ip};
                break;
        }
        addConn(root, path, obj);
        connCount++;
        if (active) activeCount++;
        totalUp += obj["Upload"].toVariant().toLongLong();
        totalDown += obj["Download"].toVariant().toLongLong();
    }

    filterNode(root, query);
    fillTree(tree, nullptr, root, sortMode, 0);

    summary->setText(tr("Connections: %1 (%2 active)  •  %3")
                         .arg(connCount)
                         .arg(activeCount)
                         .arg(fmtTraffic(totalUp, totalDown)));
}
