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
#include <QCheckBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QJsonArray>
#include <QMap>
#include <QSet>

#include <cctype>

#include "core/net/SftpClient.h"
#include "collector/CollectorClient.h"
#include "github/GitHubRepoCatalog.h"

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

// Traduit l'état opérationnel d'une source (contrat morfcollect/1) en libellé
// lisible. `suspended` (état administratif) prime pour l'affichage.
QString stateLabel(const QString& adminState, const QString& opState) {
    if (adminState == "suspended") return QStringLiteral("suspendu");
    if (adminState == "retired")   return QStringLiteral("retiré");
    if (opState == "idle")         return QStringLiteral("prêt");
    if (opState == "waiting")      return QStringLiteral("à jour");
    if (opState == "collecting")   return QStringLiteral("collecte en cours…");
    if (opState == "auth_failed")  return QStringLiteral("identifiants refusés");
    if (opState == "unreachable")  return QStringLiteral("serveur injoignable");
    if (opState == "error")        return QStringLiteral("erreur");
    return opState.isEmpty() ? QStringLiteral("—") : opState;
}

// Jeton de couleur (thème) pour l'état, comme la pastille des sites.
const char* stateColorToken(const QString& adminState, const QString& opState) {
    if (adminState == "suspended" || adminState == "retired") return "warn";
    if (opState == "waiting" || opState == "idle")            return "ok";
    if (opState == "collecting")                              return "warn";
    if (opState == "auth_failed" || opState == "unreachable" || opState == "error")
        return "danger";
    return "neutral";
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

void fillUrlCombo(QComboBox* box, const QMap<QString, QString>& seen,
                  const QString& current, const QString& autoLabel) {
    box->addItem(autoLabel, QString());
    for (auto it = seen.constBegin(); it != seen.constEnd(); ++it) {
        const QString app = it.value().isEmpty() ? QStringLiteral("service") : it.value();
        box->addItem(QStringLiteral("%1 - %2").arg(app, it.key()), it.key());
    }
    const int idx = box->findData(current);
    box->setCurrentIndex(idx >= 0 ? idx : 0);
}

} // namespace

// ---------------------------------------------------------------------------
SettingsDialog::SettingsDialog(const Config& config, const QString& discoveredUrl,
                               const QMap<QString, QString>& discoveredCollectors,
                               const QMap<QString, QString>& discoveredAnalytics, QWidget* parent)
    : QDialog(parent), config_(config), discoveredUrl_(discoveredUrl),
      discoveredCollectors_(discoveredCollectors),
      discoveredAnalytics_(discoveredAnalytics) {
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

    // Si un collecteur est déjà connu (découvert par la fenêtre principale, ou
    // adresse en config), on remplit le panneau d'état sans attendre un clic sur
    // « Se connecter ». Différé pour ne pas bloquer l'ouverture de la fenêtre.
    if (!collectorUrl().isEmpty())
        QTimer::singleShot(0, this, &SettingsDialog::refreshCollector);
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
    auto* githubPage = new QWidget;
    auto* githubLayout = new QVBoxLayout(githubPage);
    githubLayout->setSpacing(12);
    buildGithubGroup(githubLayout);
    githubLayout->addStretch();
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

    // Meme geste que l'onglet GitHub : enregistrer et confier la collecte au
    // Pi, sinon SiteWatch garde la config locale et morfCollector n'agit pas.
    auto* sitesPushBtn = new QPushButton(QStringLiteral("Envoyer la config"));
    sitesPushBtn->setToolTip(
        QStringLiteral("Enregistre et pousse sites + GitHub vers morfCollector."));
    connect(sitesPushBtn, &QPushButton::clicked, this, &SettingsDialog::requestPushAndAccept);
    rightCol->addWidget(sitesPushBtn);
    rightCol->addStretch();

    sitesLayout->addLayout(rightCol, 1);
    localLayout->addWidget(sitesBox);

    tabs->addTab(localPage, "Sites");
    tabs->addTab(githubPage, "GitHub");
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

    if (githubCollectorPick_ && collectorEdit_) {
        connect(githubCollectorPick_, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int) {
            collectorEdit_->setText(githubCollectorPick_->currentData().toString());
            if (collectorPick_) {
                const int i = collectorPick_->findData(githubCollectorPick_->currentData());
                if (i >= 0)
                    collectorPick_->setCurrentIndex(i);
            }
        });
    }
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
    if (githubAnalyticsEdit_)
        config_.analyticsUrl = githubAnalyticsEdit_->text().trimmed().toStdString();
    if (githubEnabled_) {
        config_.github.enabled = githubEnabled_->isChecked();
        config_.github.owner = githubOwner_->text().trimmed().toStdString();
        const QString tok = githubToken_->text().trimmed();
        if (!tok.isEmpty())
            config_.github.token = tok.toStdString();
        config_.github.dailyAt = githubTime_->time().toString("HH:mm").toStdString();
        config_.github.repositories.clear();
        for (int i = 0; i < githubRepos_->rowCount(); ++i) {
            auto* nameItem = githubRepos_->item(i, 1);
            if (!nameItem)
                nameItem = githubRepos_->item(i, 0);
            if (!nameItem || nameItem->text().trimmed().isEmpty())
                continue;
            GitHubRepoConfig r;
            r.name = nameItem->text().trimmed().toStdString();
            auto* chk = qobject_cast<QCheckBox*>(githubRepos_->cellWidget(i, 0));
            if (!chk)
                chk = qobject_cast<QCheckBox*>(githubRepos_->cellWidget(i, 1));
            r.enabled = chk ? chk->isChecked() : true;
            config_.github.repositories.push_back(r);
        }
    }
    accept();
}

void SettingsDialog::buildGithubGroup(QVBoxLayout* root) {
    auto* intro = new QLabel(QStringLiteral(
        "Les dépôts GitHub font partie de la présence en ligne. "
        "Listez le compte, cochez ceux à suivre, puis envoyez la config : "
        "morfCollector collectera seul à l'heure indiquée. "
        "Le jeton (fine-grained PAT, Administration en lecture) reste local "
        "et n'est jamais écrit dans le manifeste."));
    intro->setWordWrap(true);
    intro->setProperty("muted", true);
    root->addWidget(intro);

    auto* box = new QGroupBox(QStringLiteral("Compte GitHub"));
    auto* form = new QFormLayout(box);

    githubEnabled_ = new QCheckBox(QStringLiteral("Collecter les métriques GitHub"));
    githubEnabled_->setChecked(config_.github.enabled);
    form->addRow(githubEnabled_);

    githubOwner_ = new QLineEdit(QString::fromStdString(config_.github.owner));
    githubOwner_->setPlaceholderText(QStringLiteral("morfredus"));
    addRow(form, QStringLiteral("Propriétaire :"), githubOwner_);

    githubToken_ = new QLineEdit;
    githubToken_->setEchoMode(QLineEdit::Password);
    githubToken_->setPlaceholderText(config_.github.token.empty()
        ? QStringLiteral("PAT (jamais affiché ensuite)")
        : QStringLiteral("conservé - laisser vide pour ne pas changer"));
    addRow(form, QStringLiteral("Jeton GitHub :"),
           fieldWith(githubToken_, makeEye(githubToken_)));

    githubTime_ = new QTimeEdit;
    githubTime_->setDisplayFormat(QStringLiteral("HH:mm"));
    QTime t = QTime::fromString(QString::fromStdString(config_.github.dailyAt),
                                QStringLiteral("HH:mm"));
    if (!t.isValid())
        t = QTime(2, 30);
    githubTime_->setTime(t);
    addRow(form, QStringLiteral("Collecte quotidienne à :"), githubTime_);

    githubCollectorPick_ = new QComboBox;
    fillUrlCombo(githubCollectorPick_, discoveredCollectors_,
                 QString::fromStdString(config_.collectorUrl),
                 QStringLiteral("Automatique (1er détecté)"));
    addRow(form, QStringLiteral("Collecteur :"), githubCollectorPick_);
    githubAnalyticsPick_ = new QComboBox;
    fillUrlCombo(githubAnalyticsPick_, discoveredAnalytics_,
                 QString::fromStdString(config_.analyticsUrl),
                 QStringLiteral("Automatique (même hôte que le collecteur)"));
    addRow(form, QStringLiteral("Analyses avancées :"), githubAnalyticsPick_);
    githubAnalyticsEdit_ = new QLineEdit(QString::fromStdString(config_.analyticsUrl));
    githubAnalyticsEdit_->setPlaceholderText(
        QStringLiteral("http://pi4dev:8799  (vide = même hôte que le collecteur)"));
    addRow(form, QStringLiteral("URL d'analyse :"), githubAnalyticsEdit_);
    connect(githubAnalyticsPick_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        githubAnalyticsEdit_->setText(githubAnalyticsPick_->currentData().toString());
    });
    root->addWidget(box);

    githubRepos_ = new QTableWidget(0, 3);
    githubRepos_->setHorizontalHeaderLabels(
        {QStringLiteral("Suivre"), QStringLiteral("Dépôt"), QStringLiteral("Accès")});
    githubRepos_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    githubRepos_->verticalHeader()->setVisible(false);
    githubRepos_->setSelectionBehavior(QAbstractItemView::SelectRows);
    githubRepos_->setMinimumHeight(220);
    for (const GitHubRepoConfig& r : config_.github.repositories)
        addGithubRepoRow(QString::fromStdString(r.name), r.enabled, QString());
    root->addWidget(githubRepos_);

    githubListStatus_ = new QLabel;
    githubListStatus_->setProperty("muted", true);
    githubListStatus_->setWordWrap(true);
    githubListStatus_->setText(QStringLiteral(
        "Cliquez sur « Lister les dépôts » pour les charger depuis GitHub, "
        "puis cochez ceux à suivre."));
    root->addWidget(githubListStatus_);

    auto* btnRow = new QHBoxLayout;
    auto* listBtn = new QPushButton(QStringLiteral("Lister les dépôts"));
    listBtn->setToolTip(QStringLiteral(
        "Interroge GitHub (compte ou organisation) sans envoyer le jeton au collecteur."));
    connect(listBtn, &QPushButton::clicked, this, &SettingsDialog::onListGithubRepos);
    auto* addBtn = new QPushButton(QStringLiteral("+ Nom manuel"));
    auto* remBtn = new QPushButton(QStringLiteral("− Retirer"));
    connect(addBtn, &QPushButton::clicked, this, [this] {
        addGithubRepoRow(QString(), true, QString());
        githubRepos_->editItem(githubRepos_->item(githubRepos_->rowCount() - 1, 1));
    });
    connect(remBtn, &QPushButton::clicked, this, [this] {
        const int row = githubRepos_->currentRow();
        if (row >= 0)
            githubRepos_->removeRow(row);
    });
    auto* pushBtn = new QPushButton(QStringLiteral("Envoyer la config"));
    pushBtn->setToolTip(QStringLiteral(
        "Enregistre et pousse sites + GitHub vers morfCollector, qui collectera ensuite seul."));
    connect(pushBtn, &QPushButton::clicked, this, &SettingsDialog::requestPushAndAccept);
    btnRow->addWidget(listBtn);
    btnRow->addWidget(addBtn);
    btnRow->addWidget(remBtn);
    btnRow->addStretch();
    btnRow->addWidget(pushBtn);
    root->addLayout(btnRow);
}

void SettingsDialog::requestPushAndAccept() {
    pushRequested_ = true;
    onAccept();
}

QMap<QString, bool> SettingsDialog::githubSelection() const {
    QMap<QString, bool> sel;
    if (!githubRepos_)
        return sel;
    for (int i = 0; i < githubRepos_->rowCount(); ++i) {
        auto* nameItem = githubRepos_->item(i, 1);
        if (!nameItem || nameItem->text().trimmed().isEmpty())
            continue;
        auto* chk = qobject_cast<QCheckBox*>(githubRepos_->cellWidget(i, 0));
        sel.insert(nameItem->text().trimmed(), chk ? chk->isChecked() : true);
    }
    return sel;
}

void SettingsDialog::addGithubRepoRow(const QString& name, bool enabled,
                                      const QString& access) {
    const int row = githubRepos_->rowCount();
    githubRepos_->insertRow(row);
    auto* chk = new QCheckBox;
    chk->setChecked(enabled);
    githubRepos_->setCellWidget(row, 0, chk);
    auto* nameItem = new QTableWidgetItem(name);
    githubRepos_->setItem(row, 1, nameItem);
    auto* accItem = new QTableWidgetItem(access);
    accItem->setFlags(accItem->flags() & ~Qt::ItemIsEditable);
    githubRepos_->setItem(row, 2, accItem);
}

void SettingsDialog::onListGithubRepos() {
    const QString owner = githubOwner_->text().trimmed();
    QString token = githubToken_->text().trimmed();
    if (token.isEmpty())
        token = QString::fromStdString(config_.github.token);
    if (owner.isEmpty()) {
        githubListStatus_->setText(QStringLiteral("Indiquez d'abord le propriétaire GitHub."));
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    githubListStatus_->setText(QStringLiteral("Interrogation de GitHub…"));
    QApplication::processEvents();
    const GitHubRepoCatalog::Result listed = GitHubRepoCatalog::listOwned(owner, token);
    QApplication::restoreOverrideCursor();

    if (!listed.ok) {
        githubListStatus_->setText(listed.error);
        return;
    }

    const QMap<QString, bool> prev = githubSelection();
    githubRepos_->setRowCount(0);

    QSet<QString> seen;
    for (const GitHubRepoCatalog::Repo& r : listed.repos) {
        seen.insert(r.name.toLower());
        QString access = r.isPrivate ? QStringLiteral("privé") : QStringLiteral("public");
        if (r.archived)
            access += QStringLiteral(", archivé");
        const bool enabled = prev.contains(r.name) ? prev.value(r.name) : false;
        addGithubRepoRow(r.name, enabled, access);
    }

    // Un depot deja suivi mais absent de l'API reste visible pour ne pas le
    // perdre silencieusement (renomme, prive sans jeton, etc.).
    int orphans = 0;
    for (auto it = prev.constBegin(); it != prev.constEnd(); ++it) {
        if (seen.contains(it.key().toLower()))
            continue;
        addGithubRepoRow(it.key(), it.value(),
                         QStringLiteral("absent de la liste GitHub"));
        ++orphans;
    }

    githubListStatus_->setText(
        QStringLiteral("%1 dépôt(s) trouvés. Cochez ceux à suivre, puis "
                       "« Envoyer la config ».")
            .arg(listed.repos.size())
        + (orphans > 0
               ? QStringLiteral(" %1 déjà suivi(s) n'apparaissent plus chez GitHub.")
                     .arg(orphans)
               : QString()));
}

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

    // Choix du collecteur quand PLUSIEURS morfCollector sont présents sur le
    // réseau. Tous reçoivent le manifeste (redondance d'archive) ; celui choisi
    // ici devient la source de LECTURE (copies locales) et l'adresse épinglée.
    // « Automatique » = pas d'épingle : SiteWatch lit le 1er collecteur détecté.
    if (discoveredCollectors_.size() >= 2) {
        collectorPick_ = new QComboBox;
        collectorPick_->addItem("Automatique (1er détecté)", QString());
        for (auto it = discoveredCollectors_.constBegin();
             it != discoveredCollectors_.constEnd(); ++it) {
            const QString app = it.value().isEmpty() ? QStringLiteral("morfCollector") : it.value();
            collectorPick_->addItem(QStringLiteral("%1 — %2").arg(app, it.key()), it.key());
        }
        // Présélectionner l'URL déjà épinglée dans la config, si elle correspond.
        const int idx = collectorPick_->findData(QString::fromStdString(config_.collectorUrl));
        collectorPick_->setCurrentIndex(idx >= 0 ? idx : 0);
        addRow(form, "Collecteur à utiliser :", collectorPick_);
    }

    collectorEdit_ = new QLineEdit(QString::fromStdString(config_.collectorUrl));
    collectorEdit_->setPlaceholderText("http://pi4fred:8792  (vide = découverte automatique)");
    // Le sélecteur remplit le champ : choisir un collecteur l'épingle comme source
    // de lecture ; « Automatique » vide le champ. On rafraîchit alors l'état.
    if (collectorPick_) {
        connect(collectorPick_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this](int) {
            collectorEdit_->setText(collectorPick_->currentData().toString());
            refreshCollector();
        });
    }
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
    connect(pushBtn, &QPushButton::clicked, this, &SettingsDialog::requestPushAndAccept);

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
        "Si le Pi était éteint, la collecte a lieu au démarrage suivant. "
        "Le même bouton existe dans les onglets Sites et GitHub : un envoi couvre les deux familles.");
    schedHint->setProperty("muted", true);
    schedHint->setWordWrap(true);
    v->addWidget(schedHint);

    collectorState_ = new QLabel("État inconnu — cliquez sur « Se connecter ».");
    collectorState_->setProperty("muted", true);
    collectorState_->setWordWrap(true);
    v->addWidget(collectorState_);

    // --- État du service et sites confiés -----------------------------------
    // La partie basse récapitule ce que fait réellement morfCollector : pour
    // chaque site confié, son état côté collecteur et le nombre de fichiers
    // téléchargés (distinct du cache local de SiteWatch).
    auto* sitesHeading = new QLabel("<b>Sites confiés à la collecte</b>");
    v->addWidget(sitesHeading);

    collectorSites_ = new QTableWidget(0, 4);
    collectorSites_->setHorizontalHeaderLabels(
        QStringList{ "Site", "État", "Fichiers", "Taille" });
    collectorSites_->verticalHeader()->setVisible(false);
    collectorSites_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    collectorSites_->setSelectionMode(QAbstractItemView::NoSelection);
    collectorSites_->setFocusPolicy(Qt::NoFocus);
    collectorSites_->setMaximumHeight(140);
    collectorSites_->horizontalHeader()->setStretchLastSection(false);
    collectorSites_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    collectorSites_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    collectorSites_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    collectorSites_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    v->addWidget(collectorSites_);

    // Actions globales : collecter tout de suite, ou repartir de zéro.
    auto* svcRow = new QHBoxLayout;
    auto* collectNowBtn = new QPushButton("Collecter maintenant");
    collectNowBtn->setToolTip(
        "Lance une collecte immédiate de tous les sites, sans attendre l'heure programmée.");
    auto* resetBtn = new QPushButton("Réinitialiser");
    resetBtn->setToolTip(
        "Efface les copies conservées sur le collecteur puis les re-télécharge "
        "depuis l'hébergeur (utile si des fichiers manquent).");
    svcRow->addWidget(collectNowBtn);
    svcRow->addWidget(resetBtn);
    svcRow->addStretch();
    v->addLayout(svcRow);
    connect(collectNowBtn, &QPushButton::clicked, this, &SettingsDialog::collectNowAll);
    connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::resetCollector);

    auto* sep = new QLabel("<b>Copies conservées</b>");
    v->addWidget(sep);

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
        collectorSites_->setRowCount(0);
        collectorFiles_->clear();
        return;
    }

    const CollectorClient::Reply st = CollectorClient::getStatus(url);
    if (!st.ok()) {
        collectorState_->setText("Collecteur injoignable à " + url.toHtmlEscaped() + ".");
        collectorSites_->setRowCount(0);
        collectorFiles_->clear();
        return;
    }

    // Synthèse : identité du service, version, configuration active et totaux.
    const QJsonObject m = st.json.value("metrics").toObject();
    const qint64 lastTs = static_cast<qint64>(m.value("last_collect_ts").toDouble());
    const QString lastCollect = lastTs > 0
        ? QDateTime::fromSecsSinceEpoch(lastTs).toString("dd/MM/yyyy HH:mm")
        : QStringLiteral("jamais");
    const QString dailyAt = collectorTime_->time().toString("HH:mm");
    collectorState_->setText(QString(
        "Service <b>%1</b> v%2 à %3 — collecte quotidienne à %4.<br>"
        "%5 fichier(s) téléchargé(s), %6 conservés, %7 source(s). "
        "Dernière collecte : %8.")
        .arg(st.json.value("host").toString().toHtmlEscaped())
        .arg(st.json.value("version").toString().toHtmlEscaped())
        .arg(url.toHtmlEscaped())
        .arg(dailyAt)
        .arg(m.value("objects").toInt())
        .arg(QString::number(m.value("bytes").toDouble() / (1024.0 * 1024.0), 'f', 1) + " Mo")
        .arg(m.value("sources").toInt())
        .arg(lastCollect));

    // Tableau des sites confiés : état côté collecteur + fichiers téléchargés.
    const CollectorClient::Reply sr = CollectorClient::getSources(url);
    const QJsonArray sources = sr.json.value("sources").toArray();
    collectorSites_->setRowCount(sources.size());
    for (int row = 0; row < sources.size(); ++row) {
        const QJsonObject s = sources.at(row).toObject();
        const QString admin = s.value("admin_state").toString();
        const QString oper  = s.value("operational_state").toString();

        auto* siteItem = new QTableWidgetItem(s.value("label").toString());
        auto* stateItem = new QTableWidgetItem(stateLabel(admin, oper));
        stateItem->setForeground(QColor(Theme::instance().color(stateColorToken(admin, oper))));
        const QString err = s.value("last_error").toString();
        if (!err.isEmpty())
            stateItem->setToolTip(err);
        auto* filesItem = new QTableWidgetItem(
            QString::number(static_cast<qint64>(s.value("objects").toDouble())));
        filesItem->setTextAlignment(Qt::AlignCenter);
        auto* sizeItem = new QTableWidgetItem(
            humanSize(static_cast<qint64>(s.value("bytes").toDouble())));
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        collectorSites_->setItem(row, 0, siteItem);
        collectorSites_->setItem(row, 1, stateItem);
        collectorSites_->setItem(row, 2, filesItem);
        collectorSites_->setItem(row, 3, sizeItem);
    }

    refreshCollectorFiles();
}

// (Re)dépose les secrets de chaque site vers le coffre du collecteur. Idempotent :
// le collecteur écrase la valeur pour une même référence. Sert avant une collecte
// forcée pour garantir que le coffre n'est pas vide (redéploiement, réinitialisation).
void SettingsDialog::pushAllCredentials(const QString& url) {
    for (const SiteConfig& s : config_.sites) {
        QJsonObject secret;
        if (!s.user.empty())     secret["user"]     = QString::fromStdString(s.user);
        if (!s.password.empty()) secret["password"] = QString::fromStdString(s.password);
        if (!s.keyFile.empty())  secret["key_file"] = QString::fromStdString(s.keyFile);
        if (secret.isEmpty())
            continue;
        const QString ref = QStringLiteral("sw-") + QString::fromStdString(s.id);
        CollectorClient::pushCredentials(url, ref, secret);
    }
}

void SettingsDialog::collectNowAll() {
    const QString url = collectorUrl();
    if (url.isEmpty()) {
        collectorState_->setText("Aucun collecteur joignable.");
        return;
    }
    const CollectorClient::Reply sr = CollectorClient::getSources(url);
    const QJsonArray sources = sr.json.value("sources").toArray();
    if (sources.isEmpty()) {
        QMessageBox::information(this, "Collecter maintenant",
            "Le collecteur ne connaît encore aucun site. Cliquez sur « Envoyer la config » "
            "pour lui confier vos sites, puis relancez la collecte.");
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    // Le coffre a pu être vidé (redéploiement) : on redépose les secrets d'abord.
    pushAllCredentials(url);
    for (const QJsonValue& v : sources)
        CollectorClient::collectNow(url, v.toObject().value("source_id").toString());
    QApplication::restoreOverrideCursor();
    collectorState_->setText("Collecte lancée… actualisation dans quelques secondes.");
    // La collecte est asynchrone côté collecteur ; on relit l'état après un délai.
    QTimer::singleShot(3500, this, &SettingsDialog::refreshCollector);
}

void SettingsDialog::resetCollector() {
    const QString url = collectorUrl();
    if (url.isEmpty()) {
        collectorState_->setText("Aucun collecteur joignable.");
        return;
    }
    const CollectorClient::Reply sr = CollectorClient::getSources(url);
    const QJsonArray sources = sr.json.value("sources").toArray();
    if (sources.isEmpty()) {
        QMessageBox::information(this, "Réinitialiser",
            "Le collecteur ne connaît encore aucun site. Cliquez sur « Envoyer la config » "
            "d'abord.");
        return;
    }
    if (QMessageBox::question(this, "Réinitialiser morfCollector",
            "Effacer TOUTES les copies conservées sur le collecteur puis les re-télécharger "
            "depuis l'hébergeur ?\n\nLes fichiers déjà présents dans le cache local de "
            "SiteWatch ne sont pas touchés.")
            != QMessageBox::Yes)
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    pushAllCredentials(url);   // garantit un coffre non vide avant de re-collecter
    for (const QJsonValue& v : sources) {
        const QString id = v.toObject().value("source_id").toString();
        CollectorClient::deleteSourceObjects(url, id);   // vide les copies -> lève la déduplication
        CollectorClient::collectNow(url, id);            // re-télécharge tout ce qui est dispo
    }
    QApplication::restoreOverrideCursor();
    collectorState_->setText("Réinitialisation lancée… actualisation dans quelques secondes.");
    QTimer::singleShot(4000, this, &SettingsDialog::refreshCollector);
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
