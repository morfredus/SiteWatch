/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "ui/SettingsDialog.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

#include <QColor>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QStyle>
#include <QComboBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QTimeEdit>
#include <QTime>
#include <QMessageBox>
#include <QAbstractItemView>
#include <QJsonArray>

#include <cctype>

#include "core/net/SftpClient.h"
#include "collector/CollectorClient.h"

// ---------------------------------------------------------------------------
namespace {

// Assemble un champ de saisie + un widget annexe (bouton / œil).
QWidget* fieldWith(QLineEdit* edit, QWidget* extra) {
    auto* w = new QWidget;
    auto* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    h->addWidget(edit, 1);
    h->addWidget(extra);
    return w;
}

// Ajoute une ligne au formulaire avec un libellé de largeur fixe (alignement).
void addRow(QFormLayout* form, const QString& label, QWidget* field) {
    auto* l = new QLabel(label);
    l->setMinimumWidth(150);
    form->addRow(l, field);
}

// Petit bouton "œil" pour afficher/masquer un champ masqué.
QToolButton* makeEye(QLineEdit* edit) {
    auto* eye = new QToolButton;
    eye->setText(icons::ch(icons::Glyph::Eye));
    eye->setCheckable(true);
    eye->setCursor(Qt::PointingHandCursor);
    eye->setToolTip("Afficher / masquer");
    eye->setObjectName("eye");
    QObject::connect(eye, &QToolButton::toggled, edit, [edit](bool on) {
        edit->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password);
    });
    return eye;
}

QString humanSize(qint64 bytes) {
    const double mo = bytes / (1024.0 * 1024.0);
    if (mo >= 1.0) return QString::number(mo, 'f', 1).replace('.', ',') + " Mo";
    return QString::number(bytes / 1024.0, 'f', 1).replace('.', ',') + " Ko";
}

// Préfixe des fichiers de logs déduit du nom du site (points retirés, minuscules).
QString sitePrefix(const std::string& name) {
    QString r;
    for (unsigned char c : name)
        if (c != '.') r += static_cast<char>(std::tolower(c));
    return r;
}

bool fileMatches(const std::string& filename, const QString& prefix) {
    const std::string head = filename.substr(0, filename.find('.'));
    return sitePrefix(head) == prefix;
}

// Autorise l'IP publique courante sur le pare-feu SSH via l'API cPanel o2switch.
bool firewallWhitelist(const SiteConfig& site, QString& error) {
    if (site.cpanelToken.empty()) return true;
    QNetworkAccessManager nam;
    auto syncGet = [&nam](const QNetworkRequest& req, QByteArray& body) {
        QNetworkReply* r = nam.get(req);
        QEventLoop loop;
        QObject::connect(r, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        body = r->readAll();
        auto err = r->error();
        r->deleteLater();
        return err;
    };
    QByteArray ipBody;
    if (syncGet(QNetworkRequest(QUrl("https://api.ipify.org")), ipBody) != QNetworkReply::NoError) {
        error = "IP publique indéterminable.";
        return false;
    }
    const QString ip = QString::fromUtf8(ipBody).trimmed();
    QUrl url(QString("https://%1:2083/execute/SshWhitelist/add").arg(QString::fromStdString(site.host)));
    QUrlQuery q; q.addQueryItem("address", ip); q.addQueryItem("port", "22");
    url.setQuery(q);
    QNetworkRequest req(url);
    req.setRawHeader("Authorization",
        "cpanel " + QByteArray::fromStdString(site.user) + ":" + QByteArray::fromStdString(site.cpanelToken));
    QByteArray body;
    if (syncGet(req, body) != QNetworkReply::NoError) {
        error = "Jeton d'API refusé.";
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (doc.isObject() && doc.object().value("status").toInt() != 1) {
        error = QString::fromUtf8(body).left(160);
        return false;
    }
    QEventLoop wait; QTimer::singleShot(4000, &wait, &QEventLoop::quit); wait.exec();
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
SettingsDialog::SettingsDialog(const Config& config, const QString& discoveredUrl, QWidget* parent)
    : QDialog(parent), config_(config), discoveredUrl_(discoveredUrl) {
    setWindowTitle("Configuration — SiteWatch");
    setMinimumWidth(780);

    if (config_.cacheRoot.empty()) {
        const QString def = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/cache";
        config_.cacheRoot = QDir::toNativeSeparators(def).toStdString();
    }
    siteState_.assign(config_.sites.size(), 0);
    lastReport_.assign(config_.sites.size(), QString());

    buildUi();
    refreshSiteList();
    updateSummary();
    if (!config_.sites.empty())
        sitesList_->setCurrentRow(0);
    else
        setFormEnabled(false);
}

void SettingsDialog::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 12);
    root->setSpacing(12);

    // Deux onglets : le fonctionnement LOCAL de SiteWatch d'un côté, tout ce qui
    // concerne morfCollector (réseau) de l'autre.
    auto* tabs = new QTabWidget;
    auto* localPage = new QWidget;
    auto* localLayout = new QVBoxLayout(localPage);
    localLayout->setSpacing(12);
    auto* collectorPage = new QWidget;
    auto* collectorLayout = new QVBoxLayout(collectorPage);
    collectorLayout->setSpacing(12);

    // --- Emplacement des données SiteWatch ---
    auto* dataBox = new QGroupBox("Stockage");
    auto* dataForm = new QFormLayout(dataBox);
    cacheEdit_ = new QLineEdit(QString::fromStdString(config_.cacheRoot));
    auto* browseCache = new QPushButton("Parcourir…");
    auto* defCache = new QPushButton("Par défaut");
    auto* cacheRow = new QWidget;
    auto* cacheRowL = new QHBoxLayout(cacheRow);
    cacheRowL->setContentsMargins(0, 0, 0, 0);
    cacheRowL->addWidget(cacheEdit_, 1);
    cacheRowL->addWidget(browseCache);
    cacheRowL->addWidget(defCache);
    addRow(dataForm, "Cache des données :", cacheRow);
    const QString configBase = QDir::toNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    auto* hint = new QLabel("La configuration de SiteWatch est enregistrée dans : " + configBase);
    hint->setProperty("muted", true);
    hint->setWordWrap(true);
    dataForm->addRow("", hint);
    connect(browseCache, &QPushButton::clicked, this, &SettingsDialog::onBrowseCache);
    connect(defCache, &QPushButton::clicked, this, [this] {
        cacheEdit_->setText(QDir::toNativeSeparators(
            QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/cache"));
    });
    localLayout->addWidget(dataBox);

    // --- morfCollector : connexion, planification, état et copies conservées ---
    buildCollectorGroup(collectorLayout);
    collectorLayout->addStretch();

    // --- Sites ---
    auto* sitesBox = new QGroupBox("Sites");
    auto* sitesLayout = new QHBoxLayout(sitesBox);

    auto* leftCol = new QVBoxLayout;
    sitesList_ = new QListWidget;
    sitesList_->setMaximumWidth(200);
    leftCol->addWidget(sitesList_);
    auto* btnRow = new QHBoxLayout;
    auto* addBtn = new QPushButton("+ Ajouter");
    auto* remBtn = new QPushButton("− Supprimer");
    btnRow->addWidget(addBtn);
    btnRow->addWidget(remBtn);
    leftCol->addLayout(btnRow);

    // Résumé sous la liste.
    summaryLabel_ = new QLabel;
    summaryLabel_->setObjectName("summaryBox");
    leftCol->addWidget(summaryLabel_);
    sitesLayout->addLayout(leftCol);

    // --- Colonne droite : formulaire + test ---
    auto* rightCol = new QVBoxLayout;

    auto* topForm = new QFormLayout;
    nameEdit_ = new QLineEdit;
    nameEdit_->setPlaceholderText("morfredus.fr");
    hostEdit_ = new QLineEdit;
    userEdit_ = new QLineEdit;
    addRow(topForm, "Nom du site :", nameEdit_);
    addRow(topForm, "Serveur SFTP :", hostEdit_);
    addRow(topForm, "Utilisateur :", userEdit_);
    rightCol->addLayout(topForm);

    auto* authBox = new QGroupBox("Authentification");
    auto* authForm = new QFormLayout(authBox);
    keyEdit_ = new QLineEdit;
    keyEdit_->setMinimumWidth(400);
    auto* browseKey = new QPushButton("Parcourir…");
    addRow(authForm, "Clé SSH :", fieldWith(keyEdit_, browseKey));
    passEdit_ = new QLineEdit;
    passEdit_->setEchoMode(QLineEdit::Password);
    passEye_ = makeEye(passEdit_);
    addRow(authForm, "Mot de passe :", fieldWith(passEdit_, passEye_));
    rightCol->addWidget(authBox);

    auto* botForm = new QFormLayout;
    remoteEdit_ = new QLineEdit;
    tokenEdit_ = new QLineEdit;
    tokenEdit_->setEchoMode(QLineEdit::Password);
    tokenEye_ = makeEye(tokenEdit_);
    addRow(botForm, "Dossier distant des logs :", remoteEdit_);
    addRow(botForm, "Jeton d'API cPanel :", fieldWith(tokenEdit_, tokenEye_));
    logMatchEdit_ = new QLineEdit;
    logMatchEdit_->setPlaceholderText("vide = détection automatique (o2switch)");
    addRow(botForm, "Filtre des logs (avancé) :", logMatchEdit_);
    rightCol->addLayout(botForm);

    testButton_ = new QPushButton("Tester la connexion");
    rightCol->addWidget(testButton_);
    testResult_ = new QLabel;
    testResult_->setObjectName("testResult");
    testResult_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    testResult_->setWordWrap(true);
    rightCol->addWidget(testResult_);
    rightCol->addStretch();

    sitesLayout->addLayout(rightCol, 1);
    localLayout->addWidget(sitesBox);

    tabs->addTab(localPage, "SiteWatch");
    tabs->addTab(collectorPage, "morfCollector");
    root->addWidget(tabs);

    // --- Boutons ---
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setText("Enregistrer");
    buttons->button(QDialogButtonBox::Cancel)->setText("Annuler");
    root->addWidget(buttons);

    connect(addBtn, &QPushButton::clicked, this, &SettingsDialog::onAddSite);
    connect(remBtn, &QPushButton::clicked, this, &SettingsDialog::onRemoveSite);
    connect(browseKey, &QPushButton::clicked, this, &SettingsDialog::onBrowseKey);
    connect(testButton_, &QPushButton::clicked, this, &SettingsDialog::onTestConnection);
    connect(sitesList_, &QListWidget::currentRowChanged, this, &SettingsDialog::onSiteChanged);
    connect(nameEdit_, &QLineEdit::textChanged, this, &SettingsDialog::onNameEdited);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QIcon SettingsDialog::stateIcon(int index) const {
    const char* tok = "warn";                       // à tester (orange) par défaut
    if (index >= 0 && index < static_cast<int>(siteState_.size())) {
        if (siteState_[index] == 1)      tok = "ok";      // valide (vert)
        else if (siteState_[index] == 2) tok = "danger";  // erreur (rouge)
    }
    return icons::icon(icons::Glyph::Dot, QColor(Theme::instance().color(tok)), 12);
}

void SettingsDialog::refreshSiteList() {
    loading_ = true;
    sitesList_->clear();
    for (int i = 0; i < static_cast<int>(config_.sites.size()); ++i)
        sitesList_->addItem(new QListWidgetItem(
            stateIcon(i), QString::fromStdString(config_.sites[i].name)));
    loading_ = false;
}

void SettingsDialog::updateSummary() {
    int files = 0;
    qint64 bytes = 0;
    QDateTime newest;
    const QString rootDir = QString::fromStdString(config_.cacheRoot);
    if (!rootDir.isEmpty() && QDir(rootDir).exists()) {
        QDirIterator it(rootDir, QStringList{"*.gz"}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QFileInfo fi = it.fileInfo();
            ++files;
            bytes += fi.size();
            if (!newest.isValid() || fi.lastModified() > newest) newest = fi.lastModified();
        }
    }
    summaryLabel_->setText(QString(
        "Sites configurés : %1\n"
        "Cache local : %2\n"
        "Fichiers téléchargés : %3\n"
        "Dernière synchro : %4")
        .arg(config_.sites.size())
        .arg(humanSize(bytes))
        .arg(files)
        .arg(newest.isValid() ? newest.toString("dd/MM/yyyy HH:mm") : "—"));
}

void SettingsDialog::setFormEnabled(bool on) {
    for (QWidget* w : std::initializer_list<QWidget*>{
            nameEdit_, hostEdit_, userEdit_, keyEdit_, passEdit_, passEye_,
            remoteEdit_, tokenEdit_, tokenEye_, testButton_})
        w->setEnabled(on);
}

void SettingsDialog::applyResultStyle(int state) {
    // 1 = succès (vert), 2 = erreur (rouge), autre = neutre. La couleur vient
    // du thème via QSS (#testResult[state="…"]).
    const char* s = state == 1 ? "ok" : state == 2 ? "danger" : "neutral";
    testResult_->setProperty("state", s);
    testResult_->style()->unpolish(testResult_);
    testResult_->style()->polish(testResult_);
}

void SettingsDialog::loadSiteToForm(int i) {
    if (i < 0 || i >= static_cast<int>(config_.sites.size())) return;
    const SiteConfig& s = config_.sites[i];
    loading_ = true;
    nameEdit_->setText(QString::fromStdString(s.name));
    hostEdit_->setText(QString::fromStdString(s.host));
    userEdit_->setText(QString::fromStdString(s.user));
    keyEdit_->setText(QString::fromStdString(s.keyFile));
    passEdit_->setText(QString::fromStdString(s.password));
    remoteEdit_->setText(QString::fromStdString(s.remoteLogDir));
    tokenEdit_->setText(QString::fromStdString(s.cpanelToken));
    logMatchEdit_->setText(QString::fromStdString(s.logMatch));
    testResult_->setText(lastReport_[i]);
    applyResultStyle(siteState_[i]);
    loading_ = false;
}

void SettingsDialog::commitFormToSite(int i) {
    if (i < 0 || i >= static_cast<int>(config_.sites.size())) return;
    SiteConfig& s = config_.sites[i];
    s.name         = nameEdit_->text().trimmed().toStdString();
    s.host         = hostEdit_->text().trimmed().toStdString();
    s.user         = userEdit_->text().trimmed().toStdString();
    s.keyFile      = keyEdit_->text().trimmed().toStdString();
    s.password     = passEdit_->text().toStdString();
    s.remoteLogDir = remoteEdit_->text().trimmed().toStdString();
    s.cpanelToken  = tokenEdit_->text().trimmed().toStdString();
    s.logMatch     = logMatchEdit_->text().trimmed().toStdString();
    if (s.protocol.empty()) s.protocol = "sftp";
}

void SettingsDialog::onSiteChanged(int row) {
    if (loading_) return;
    if (current_ >= 0 && current_ < static_cast<int>(config_.sites.size()))
        commitFormToSite(current_);
    current_ = row;
    if (row >= 0 && row < static_cast<int>(config_.sites.size())) {
        setFormEnabled(true);
        loadSiteToForm(row);
    } else {
        setFormEnabled(false);
    }
}

void SettingsDialog::onNameEdited(const QString& text) {
    if (loading_) return;
    if (auto* item = sitesList_->currentItem()) {
        item->setIcon(stateIcon(current_));
        item->setText(text);
    }
}

void SettingsDialog::onAddSite() {
    if (current_ >= 0) commitFormToSite(current_);
    SiteConfig s;
    s.name = "nouveau-site.fr";
    s.protocol = "sftp";
    config_.sites.push_back(s);
    siteState_.push_back(0);
    lastReport_.push_back(QString());
    refreshSiteList();
    updateSummary();
    sitesList_->setCurrentRow(static_cast<int>(config_.sites.size()) - 1);
}

void SettingsDialog::onRemoveSite() {
    if (current_ < 0 || current_ >= static_cast<int>(config_.sites.size())) return;
    config_.sites.erase(config_.sites.begin() + current_);
    siteState_.erase(siteState_.begin() + current_);
    lastReport_.erase(lastReport_.begin() + current_);
    current_ = -1;
    refreshSiteList();
    updateSummary();
    if (!config_.sites.empty())
        sitesList_->setCurrentRow(0);
    else
        setFormEnabled(false);
}

void SettingsDialog::onBrowseCache() {
    const QString start = cacheEdit_->text().isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        : cacheEdit_->text();
    const QString dir = QFileDialog::getExistingDirectory(
        this, "Choisir le dossier des données SiteWatch", start);
    if (!dir.isEmpty()) cacheEdit_->setText(QDir::toNativeSeparators(dir));
}

void SettingsDialog::onBrowseKey() {
    QString start = keyEdit_->text();
    if (start.isEmpty()) start = QDir::homePath() + "/.ssh";
    const QString file = QFileDialog::getOpenFileName(this, "Choisir la clé SSH privée", start);
    if (!file.isEmpty()) keyEdit_->setText(QDir::toNativeSeparators(file));
}

void SettingsDialog::showTestResult(int state, const QString& message) {
    testResult_->setText(message);
    applyResultStyle(state);
    if (current_ >= 0 && current_ < static_cast<int>(siteState_.size())) {
        siteState_[current_] = state;
        lastReport_[current_] = message;
        if (auto* item = sitesList_->currentItem()) {
            item->setIcon(stateIcon(current_));
            item->setText(QString::fromStdString(config_.sites[current_].name));
        }
    }
}

void SettingsDialog::onTestConnection() {
    if (current_ < 0) return;
    commitFormToSite(current_);
    const SiteConfig s = config_.sites[current_];

    QApplication::setOverrideCursor(Qt::WaitCursor);
    testResult_->setText("Test en cours…");
    applyResultStyle(0);
    QApplication::processEvents();

    QString err;
    if (!firewallWhitelist(s, err)) {
        QApplication::restoreOverrideCursor();
        showTestResult(2, "✗ Pare-feu cPanel — " + err);
        return;
    }

    SftpClient client;
    std::string cerr;
    if (!client.connect(s, cerr)) {
        QApplication::restoreOverrideCursor();
        showTestResult(2, "✓ Pare-feu cPanel\n✗ Connexion SSH — " + QString::fromStdString(cerr));
        return;
    }

    auto files = client.listLogs(s.remoteLogDir, cerr);
    client.disconnect();

    const QString prefix = sitePrefix(s.name);
    int match = 0;
    for (const auto& f : files)
        if (fileMatches(f.name, prefix)) ++match;

    QApplication::restoreOverrideCursor();
    const QString msg = QString(
        "✓ Connexion SSH\n"
        "✓ Lecture du dossier (%1 fichiers)\n"
        "✓ Préfixe détecté : %2\n"
        "✓ %3 fichier(s) pour ce site\n"
        "Dernière connexion : %4")
        .arg(files.size())
        .arg(prefix.isEmpty() ? "(nom du site vide)" : prefix)
        .arg(match)
        .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm"));
    showTestResult(1, msg);
}

void SettingsDialog::onAccept() {
    if (current_ >= 0) commitFormToSite(current_);
    config_.cacheRoot = cacheEdit_->text().trimmed().toStdString();
    config_.collectorUrl = collectorEdit_->text().trimmed().toStdString();
    config_.collectorDailyAt = collectorTime_->time().toString("HH:mm").toStdString();
    accept();
}

// --- morfCollector -----------------------------------------------------------

QString SettingsDialog::collectorUrl() const {
    const QString typed = collectorEdit_->text().trimmed();
    if (!typed.isEmpty())
        return typed;
    // Champ vide : on utilise l'URL déjà découverte par la fenêtre principale.
    // On ne relance JAMAIS de découverte ici (rebinder le port 45454 casserait
    // l'écoute permanente et ferait « disparaître » le collecteur).
    return discoveredUrl_;
}

void SettingsDialog::buildCollectorGroup(QVBoxLayout* root) {
    auto* box = new QGroupBox("morfCollector (collecte et conservation sur le réseau)");
    auto* v = new QVBoxLayout(box);

    auto* form = new QFormLayout;
    collectorEdit_ = new QLineEdit(QString::fromStdString(config_.collectorUrl));
    collectorEdit_->setPlaceholderText("http://pi4fred:8792  (vide = découverte automatique)");
    auto* connectBtn = new QPushButton("Se connecter");
    connectBtn->setToolTip("Vérifie l'adresse et affiche l'état du collecteur.");
    auto* pushBtn = new QPushButton("Envoyer la config");
    pushBtn->setToolTip("Enregistre puis pousse la configuration au collecteur.");
    auto* urlRow = new QWidget;
    auto* urlRowL = new QHBoxLayout(urlRow);
    urlRowL->setContentsMargins(0, 0, 0, 0);
    urlRowL->addWidget(collectorEdit_, 1);
    urlRowL->addWidget(connectBtn);
    urlRowL->addWidget(pushBtn);
    addRow(form, "Adresse du collecteur :", urlRow);
    connect(pushBtn, &QPushButton::clicked, this, [this] { pushRequested_ = true; onAccept(); });

    collectorTime_ = new QTimeEdit;
    collectorTime_->setDisplayFormat("HH:mm");
    collectorTime_->setTime(QTime::fromString(
        config_.collectorDailyAt.empty() ? QStringLiteral("02:00")
                                         : QString::fromStdString(config_.collectorDailyAt),
        "HH:mm"));
    addRow(form, "Collecte quotidienne à :", collectorTime_);
    v->addLayout(form);

    auto* schedHint = new QLabel(
        "morfCollector récupère les fichiers une fois par jour à cette heure (heure du Pi). "
        "Si le Pi était éteint, la collecte a lieu au démarrage suivant.");
    schedHint->setProperty("muted", true);
    schedHint->setWordWrap(true);
    v->addWidget(schedHint);

    collectorState_ = new QLabel("État inconnu — cliquez sur « Se connecter ».");
    collectorState_->setProperty("muted", true);
    collectorState_->setWordWrap(true);
    v->addWidget(collectorState_);

    // Copies conservées, par site.
    auto* filesRow = new QHBoxLayout;
    filesRow->addWidget(new QLabel("Copies du site :"));
    collectorSite_ = new QComboBox;
    for (const SiteConfig& s : config_.sites)
        collectorSite_->addItem(QString::fromStdString(s.name), QString::fromStdString(s.id));
    filesRow->addWidget(collectorSite_, 1);
    auto* refreshFilesBtn = new QPushButton("Rafraîchir les fichiers");
    refreshFilesBtn->setToolTip("Recharge la liste des fichiers conservés sur le Pi pour ce site.");
    filesRow->addWidget(refreshFilesBtn);
    v->addLayout(filesRow);

    collectorFiles_ = new QListWidget;
    collectorFiles_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    collectorFiles_->setMaximumHeight(150);
    v->addWidget(collectorFiles_);

    auto* actionRow = new QHBoxLayout;
    auto* delSel = new QPushButton("Supprimer la sélection");
    auto* delAll = new QPushButton("Supprimer toutes les copies du site");
    actionRow->addStretch();
    actionRow->addWidget(delSel);
    actionRow->addWidget(delAll);
    v->addLayout(actionRow);

    root->addWidget(box);

    connect(connectBtn, &QPushButton::clicked, this, &SettingsDialog::refreshCollector);
    connect(refreshFilesBtn, &QPushButton::clicked, this, &SettingsDialog::refreshCollectorFiles);
    connect(collectorSite_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::refreshCollectorFiles);
    connect(delSel, &QPushButton::clicked, this, [this] {
        const QString url = collectorUrl();
        if (url.isEmpty()) { collectorState_->setText("Aucun collecteur joignable."); return; }
        const auto items = collectorFiles_->selectedItems();
        if (items.isEmpty()) return;
        if (QMessageBox::question(this, "Supprimer",
                QString("Supprimer %1 fichier(s) du collecteur ? Action irréversible.")
                    .arg(items.size())) != QMessageBox::Yes) return;
        for (QListWidgetItem* it : items)
            CollectorClient::deleteObject(url, it->data(Qt::UserRole).toString());
        refreshCollector();
    });
    connect(delAll, &QPushButton::clicked, this, [this] {
        const QString url = collectorUrl();
        if (url.isEmpty() || collectorSite_->currentIndex() < 0) return;
        const QString id = collectorSite_->currentData().toString();
        if (QMessageBox::question(this, "Supprimer toutes les copies",
                "Supprimer TOUTES les copies conservées pour « "
                + collectorSite_->currentText() + " » ? Action irréversible.")
                != QMessageBox::Yes) return;
        CollectorClient::deleteSourceObjects(url, id);
        refreshCollector();
    });
}

void SettingsDialog::refreshCollector() {
    const QString url = collectorUrl();
    if (url.isEmpty()) {
        collectorState_->setText(
            "Aucun collecteur joignable. Renseignez son adresse (ex. http://pi4fred:8792) "
            "ou vérifiez qu'il tourne sur le réseau.");
        collectorFiles_->clear();
        return;
    }

    const CollectorClient::Reply st = CollectorClient::getStatus(url);
    if (!st.ok()) {
        collectorState_->setText("Collecteur injoignable à " + url.toHtmlEscaped() + ".");
        collectorFiles_->clear();
        return;
    }
    const QJsonObject m = st.json.value("metrics").toObject();
    collectorState_->setText(QString(
        "Connecté à <b>%1</b> (%2) — %3 objet(s), %4 conservés, %5 source(s).")
        .arg(st.json.value("host").toString().toHtmlEscaped())
        .arg(url.toHtmlEscaped())
        .arg(m.value("objects").toInt())
        .arg(QString::number(m.value("bytes").toDouble() / (1024.0 * 1024.0), 'f', 1) + " Mo")
        .arg(m.value("sources").toInt()));

    refreshCollectorFiles();
}

void SettingsDialog::refreshCollectorFiles() {
    collectorFiles_->clear();
    if (collectorSite_->currentIndex() < 0)
        return;
    const QString url = collectorUrl();
    if (url.isEmpty())
        return;
    const QString id = collectorSite_->currentData().toString();
    const CollectorClient::Reply o = CollectorClient::getObjects(url, id);
    for (const QJsonValue& v : o.json.value("objects").toArray()) {
        const QJsonObject j = v.toObject();
        auto* item = new QListWidgetItem(QString("%1   (%2)")
            .arg(j.value("original_name").toString())
            .arg(j.value("period").toString()));
        item->setData(Qt::UserRole, j.value("object_id").toString());
        collectorFiles_->addItem(item);
    }
}
