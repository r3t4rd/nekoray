#include "SimpleRouteEditor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QAbstractItemView>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QFontDatabase>

void SimpleRouteEditor::addListRow(QListWidget *list, const QString &text) {
    auto t = text.trimmed();
    if (t.isEmpty() || !list) return;
    for (int i = 0; i < list->count(); ++i) {
        if (list->item(i)->text().compare(t, Qt::CaseInsensitive) == 0) return;
    }
    list->addItem(t);
}

void SimpleRouteEditor::removeSelected(QListWidget *list) {
    if (!list) return;
    const auto items = list->selectedItems();
    for (auto *item: items) {
        delete list->takeItem(list->row(item));
    }
}

QJsonArray SimpleRouteEditor::collectList(QListWidget *list) {
    QJsonArray arr;
    if (!list) return arr;
    for (int i = 0; i < list->count(); ++i) arr += list->item(i)->text();
    return arr;
}

SimpleRouteEditor::ListPage SimpleRouteEditor::makeDomainPage(QWidget *parent, const QString &placeholder) {
    ListPage page;
    auto *lay = new QVBoxLayout(parent);
    page.list = new QListWidget(parent);
    page.list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    lay->addWidget(page.list, 1);
    auto *row = new QHBoxLayout;
    page.edit = new QLineEdit(parent);
    page.edit->setPlaceholderText(placeholder);
    auto *addBtn = new QPushButton(tr("Add"), parent);
    auto *delBtn = new QPushButton(tr("Remove"), parent);
    row->addWidget(page.edit, 1);
    row->addWidget(addBtn);
    row->addWidget(delBtn);
    lay->addLayout(row);
    connect(addBtn, &QPushButton::clicked, this, [=] {
        addListRow(page.list, page.edit->text());
        page.edit->clear();
    });
    connect(delBtn, &QPushButton::clicked, this, [=] { removeSelected(page.list); });
    connect(page.edit, &QLineEdit::returnPressed, this, [=] {
        addListRow(page.list, page.edit->text());
        page.edit->clear();
    });
    return page;
}

SimpleRouteEditor::ListPage SimpleRouteEditor::makeAppPage(QWidget *parent, const QString &placeholder) {
    ListPage page;
    auto *lay = new QVBoxLayout(parent);
    page.list = new QListWidget(parent);
    page.list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    lay->addWidget(page.list, 1);
    auto *row = new QHBoxLayout;
    page.edit = new QLineEdit(parent);
    page.edit->setPlaceholderText(placeholder);
    auto *addBtn = new QPushButton(tr("Add"), parent);
    auto *browseBtn = new QPushButton(tr("Browse…"), parent);
    auto *delBtn = new QPushButton(tr("Remove"), parent);
    row->addWidget(page.edit, 1);
    row->addWidget(addBtn);
    row->addWidget(browseBtn);
    row->addWidget(delBtn);
    lay->addLayout(row);
    connect(addBtn, &QPushButton::clicked, this, [=] {
        addListRow(page.list, page.edit->text());
        page.edit->clear();
    });
    connect(delBtn, &QPushButton::clicked, this, [=] { removeSelected(page.list); });
    connect(page.edit, &QLineEdit::returnPressed, this, [=] {
        addListRow(page.list, page.edit->text());
        page.edit->clear();
    });
    connect(browseBtn, &QPushButton::clicked, this, [=] {
        auto path = QFileDialog::getOpenFileName(this, tr("Select application"), QString(),
#ifdef Q_OS_WIN
                                                 tr("Programs (*.exe);;All (*.*)")
#else
                                                 tr("All (*.*)")
#endif
        );
        if (path.isEmpty()) return;
        addListRow(page.list, QFileInfo(path).fileName());
    });
    return page;
}

SimpleRouteEditor::SimpleRouteEditor(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Connection Rules"));
    resize(520, 600);

    auto *root = new QVBoxLayout(this);
    auto *hint = new QLabel(
        tr("Same data as Advanced → Custom Route.\n"
           "Loads both scheme Custom Route and Custom Route (global)."),
        this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#666; margin-bottom:6px;");
    root->addWidget(hint);

    tabs = new QTabWidget(this);

    auto *ds = new QWidget;
    directSites = makeDomainPage(ds, tr(".ru  or  example.com"));
    tabs->addTab(ds, tr("Direct sites"));

    auto *ps = new QWidget;
    proxySites = makeDomainPage(ps, tr(".ru  or  example.com"));
    tabs->addTab(ps, tr("Proxy sites"));

    auto *da = new QWidget;
    directApps = makeAppPage(da, tr("chrome.exe"));
    tabs->addTab(da, tr("Direct apps"));

    auto *pa = new QWidget;
    proxyApps = makeAppPage(pa, tr("Discord.exe"));
    tabs->addTab(pa, tr("Proxy apps"));

    auto *jsonPage = new QWidget;
    auto *jsonLay = new QVBoxLayout(jsonPage);
    jsonEdit = new QPlainTextEdit(jsonPage);
    jsonEdit->setPlaceholderText(QStringLiteral("{\n  \"rules\": []\n}"));
    jsonEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    jsonLay->addWidget(jsonEdit, 1);
    tabs->addTab(jsonPage, tr("Custom Route JSON"));

    root->addWidget(tabs, 1);

    auto *box = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    root->addWidget(box);

    connect(tabs, &QTabWidget::currentChanged, this, [=](int index) {
        if (syncing) return;
        const int jsonIndex = tabs->count() - 1;
        if (lastTabIndex == jsonIndex && index != jsonIndex) {
            if (!syncListsFromJson()) {
                syncing = true;
                tabs->setCurrentIndex(jsonIndex);
                syncing = false;
                return;
            }
        } else if (index == jsonIndex) {
            syncJsonFromLists();
        }
        lastTabIndex = index;
    });

    connect(box, &QDialogButtonBox::accepted, this, [=] {
        if (tabs->currentIndex() == tabs->count() - 1) {
            if (!syncListsFromJson()) return;
        } else {
            syncJsonFromLists();
        }
        accept();
    });
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SimpleRouteEditor::parseRulesIntoLists(const QJsonArray &rules) {
    for (const auto &v: rules) {
        if (!v.isObject()) {
            otherRulesJson << QString::fromUtf8(QJsonDocument(v.toObject()).toJson(QJsonDocument::Compact));
            continue;
        }
        auto rule = v.toObject();
        const auto outbound = rule.value("outbound").toString();
        const bool hasDomain = rule.contains("domain_suffix");
        const bool hasProc = rule.contains("process_name");
        const auto keys = rule.keys();
        const QStringList known{"domain_suffix", "process_name", "outbound"};
        bool extra = false;
        for (const auto &k: keys) {
            if (!known.contains(k)) {
                extra = true;
                break;
            }
        }
        if (extra || (hasDomain && hasProc)) {
            otherRulesJson << QString::fromUtf8(QJsonDocument(rule).toJson(QJsonDocument::Compact));
            continue;
        }
        if (hasDomain) {
            if (outbound != "proxy" && outbound != "direct" && !outbound.isEmpty()) {
                otherRulesJson << QString::fromUtf8(QJsonDocument(rule).toJson(QJsonDocument::Compact));
                continue;
            }
            auto *target = (outbound == "proxy") ? proxySites.list : directSites.list;
            for (const auto &d: rule.value("domain_suffix").toArray()) {
                addListRow(target, d.toString());
            }
            continue;
        }
        if (hasProc) {
            if (outbound != "proxy" && outbound != "direct" && !outbound.isEmpty()) {
                otherRulesJson << QString::fromUtf8(QJsonDocument(rule).toJson(QJsonDocument::Compact));
                continue;
            }
            auto *target = (outbound == "direct") ? directApps.list : proxyApps.list;
            for (const auto &p: rule.value("process_name").toArray()) {
                addListRow(target, p.toString());
            }
            continue;
        }
        otherRulesJson << QString::fromUtf8(QJsonDocument(rule).toJson(QJsonDocument::Compact));
    }
}

void SimpleRouteEditor::loadFromJson(const QString &json) {
    for (auto *list: {directSites.list, proxySites.list, directApps.list, proxyApps.list}) {
        if (list) list->clear();
    }
    otherRulesJson.clear();

    QByteArray raw = json.trimmed().toUtf8();
    if (raw.startsWith("\xEF\xBB\xBF")) raw = raw.mid(3);

    QJsonParseError err{};
    auto doc = QJsonDocument::fromJson(raw, &err);
    QJsonArray rules;
    if (doc.isObject()) {
        rules = doc.object().value("rules").toArray();
    } else if (doc.isArray()) {
        rules = doc.array();
    }
    parseRulesIntoLists(rules);
    syncJsonFromLists();
}

void SimpleRouteEditor::syncJsonFromLists() {
    if (!jsonEdit) return;
    syncing = true;
    jsonEdit->setPlainText(toJson());
    syncing = false;
}

bool SimpleRouteEditor::syncListsFromJson() {
    if (!jsonEdit) return true;
    auto text = jsonEdit->toPlainText().trimmed();
    if (text.isEmpty()) text = QStringLiteral("{\"rules\":[]}");

    QJsonParseError err{};
    auto doc = QJsonDocument::fromJson(text.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || (!doc.isObject() && !doc.isArray())) {
        QMessageBox::warning(this, tr("Custom Route JSON"),
                             tr("Invalid JSON: %1").arg(err.errorString()));
        return false;
    }

    for (auto *list: {directSites.list, proxySites.list, directApps.list, proxyApps.list}) {
        if (list) list->clear();
    }
    otherRulesJson.clear();
    if (doc.isObject()) {
        parseRulesIntoLists(doc.object().value("rules").toArray());
    } else {
        parseRulesIntoLists(doc.array());
    }
    return true;
}

QString SimpleRouteEditor::toJson() const {
    QJsonArray rules;

    auto pushDomain = [&](QListWidget *list, const char *outbound) {
        auto arr = collectList(list);
        if (!arr.isEmpty()) {
            rules += QJsonObject{{"domain_suffix", arr}, {"outbound", QString::fromUtf8(outbound)}};
        }
    };
    auto pushProc = [&](QListWidget *list, const char *outbound) {
        auto arr = collectList(list);
        if (!arr.isEmpty()) {
            rules += QJsonObject{{"outbound", QString::fromUtf8(outbound)}, {"process_name", arr}};
        }
    };

    pushDomain(directSites.list, "direct");
    pushDomain(proxySites.list, "proxy");
    pushProc(directApps.list, "direct");
    pushProc(proxyApps.list, "proxy");

    for (const auto &raw: otherRulesJson) {
        auto d = QJsonDocument::fromJson(raw.toUtf8());
        if (d.isObject()) rules += d.object();
    }
    return QString::fromUtf8(QJsonDocument(QJsonObject{{"rules", rules}}).toJson(QJsonDocument::Indented));
}
