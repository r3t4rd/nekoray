#include "SimpleRouteEditor.h"

#include "main/NekoGui_Utils.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QMessageBox>

static void addListRow(QListWidget *list, const QString &text) {
    auto t = text.trimmed();
    if (t.isEmpty()) return;
    for (int i = 0; i < list->count(); ++i) {
        if (list->item(i)->text().compare(t, Qt::CaseInsensitive) == 0) return;
    }
    list->addItem(t);
}

SimpleRouteEditor::SimpleRouteEditor(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Connection Rules"));
    resize(440, 520);

    auto *root = new QVBoxLayout(this);
    auto *hint = new QLabel(tr("Direct: sites that bypass the proxy.\nProxy apps: programs that always go through the proxy."), this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#666; margin-bottom:6px;");
    root->addWidget(hint);

    auto *tabs = new QTabWidget(this);

    // Domains tab
    auto *domPage = new QWidget;
    auto *domLay = new QVBoxLayout(domPage);
    domainList = new QListWidget(domPage);
    domainList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    domLay->addWidget(domainList, 1);
    auto *domRow = new QHBoxLayout;
    domainEdit = new QLineEdit(domPage);
    domainEdit->setPlaceholderText(tr(".ru  or  example.com"));
    auto *addDom = new QPushButton(tr("Add"), domPage);
    auto *delDom = new QPushButton(tr("Remove"), domPage);
    domRow->addWidget(domainEdit, 1);
    domRow->addWidget(addDom);
    domRow->addWidget(delDom);
    domLay->addLayout(domRow);
    tabs->addTab(domPage, tr("Direct sites"));

    // Processes tab
    auto *procPage = new QWidget;
    auto *procLay = new QVBoxLayout(procPage);
    processList = new QListWidget(procPage);
    processList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    procLay->addWidget(processList, 1);
    auto *procRow = new QHBoxLayout;
    processEdit = new QLineEdit(procPage);
    processEdit->setPlaceholderText(tr("Discord.exe"));
    auto *addProc = new QPushButton(tr("Add"), procPage);
    auto *delProc = new QPushButton(tr("Remove"), procPage);
    auto *browseProc = new QPushButton(tr("Browse…"), procPage);
    procRow->addWidget(processEdit, 1);
    procRow->addWidget(addProc);
    procRow->addWidget(browseProc);
    procRow->addWidget(delProc);
    procLay->addLayout(procRow);
    tabs->addTab(procPage, tr("Proxy apps"));

    root->addWidget(tabs, 1);

    auto *box = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    root->addWidget(box);

    connect(addDom, &QPushButton::clicked, this, &SimpleRouteEditor::addDomain);
    connect(delDom, &QPushButton::clicked, this, &SimpleRouteEditor::removeDomain);
    connect(domainEdit, &QLineEdit::returnPressed, this, &SimpleRouteEditor::addDomain);
    connect(addProc, &QPushButton::clicked, this, &SimpleRouteEditor::addProcess);
    connect(delProc, &QPushButton::clicked, this, &SimpleRouteEditor::removeProcess);
    connect(processEdit, &QLineEdit::returnPressed, this, &SimpleRouteEditor::addProcess);
    connect(browseProc, &QPushButton::clicked, this, [=] {
        auto path = QFileDialog::getOpenFileName(this, tr("Select application"), QString(),
#ifdef Q_OS_WIN
                                                 tr("Programs (*.exe);;All (*.*)")
#else
                                                 tr("All (*.*)")
#endif
        );
        if (path.isEmpty()) return;
        addListRow(processList, QFileInfo(path).fileName());
    });
    connect(box, &QDialogButtonBox::accepted, this, &SimpleRouteEditor::onAccept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SimpleRouteEditor::addDomain() {
    auto t = domainEdit->text().trimmed();
    if (t.isEmpty()) return;
    if (!t.startsWith(".") && !t.contains("://") && t.contains(".")) {
        // keep as-is; user may want full domain. For suffix list, leading dot is conventional.
    }
    addListRow(domainList, t);
    domainEdit->clear();
}

void SimpleRouteEditor::removeDomain() {
    const auto items = domainList->selectedItems();
    for (auto *item: items) {
        delete domainList->takeItem(domainList->row(item));
    }
}

void SimpleRouteEditor::addProcess() {
    addListRow(processList, processEdit->text());
    processEdit->clear();
}

void SimpleRouteEditor::removeProcess() {
    const auto items = processList->selectedItems();
    for (auto *item: items) {
        delete processList->takeItem(processList->row(item));
    }
}

void SimpleRouteEditor::loadFromJson(const QString &json) {
    domainList->clear();
    processList->clear();
    otherRulesJson.clear();

    auto doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) return;
    auto rules = doc.object().value("rules").toArray();
    for (const auto &v: rules) {
        auto rule = v.toObject();
        auto outbound = rule.value("outbound").toString();
        if (rule.contains("domain_suffix") && (outbound == "direct" || outbound.isEmpty())) {
            for (const auto &d: rule.value("domain_suffix").toArray()) {
                addListRow(domainList, d.toString());
            }
            // if rule also has other matchers, keep remainder? For simplicity absorb domains only.
            continue;
        }
        if (rule.contains("process_name") && (outbound == "proxy" || outbound.isEmpty())) {
            for (const auto &p: rule.value("process_name").toArray()) {
                addListRow(processList, p.toString());
            }
            continue;
        }
        otherRulesJson << QString::fromUtf8(QJsonDocument(rule).toJson(QJsonDocument::Compact));
    }
}

QString SimpleRouteEditor::toJson() const {
    QJsonArray rules;
    QJsonArray domains;
    for (int i = 0; i < domainList->count(); ++i) domains += domainList->item(i)->text();
    if (!domains.isEmpty()) {
        rules += QJsonObject{{"domain_suffix", domains}, {"outbound", "direct"}};
    }
    QJsonArray procs;
    for (int i = 0; i < processList->count(); ++i) procs += processList->item(i)->text();
    if (!procs.isEmpty()) {
        rules += QJsonObject{{"outbound", "proxy"}, {"process_name", procs}};
    }
    for (const auto &raw: otherRulesJson) {
        auto d = QJsonDocument::fromJson(raw.toUtf8());
        if (d.isObject()) rules += d.object();
    }
    return QString::fromUtf8(QJsonDocument(QJsonObject{{"rules", rules}}).toJson(QJsonDocument::Indented));
}

void SimpleRouteEditor::onAccept() {
    accept();
}
