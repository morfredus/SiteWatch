/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QMap>
#include <QWidget>
#include <memory>
#include "config/Config.h"

class QTableWidget;
class QLabel;
class QPushButton;
class QComboBox;
class GitHubAuthorityStore;

class GitHubPage : public QWidget {
    Q_OBJECT
public:
    explicit GitHubPage(QWidget* parent = nullptr);
    ~GitHubPage() override;

    void setConfig(const Config& cfg);
    void setCollectorUrl(const QString& url);
    void setAnalyticsUrl(const QString& url);
    void setDiscoveredPeers(const QMap<QString, QString>& collectors,
                            const QMap<QString, QString>& analytics);
    void setNotice(const QString& text);
    void refresh();
    void publishToAnalytics();
    void reconcileFromCollector();

signals:
    void openAnalyticsRequested();
    void collectorChosen(const QString& url);
    void analyticsChosen(const QString& url);
    void pushConfigRequested();

private:
    bool ensureStore();
    void collectNow();
    void catchUpFromCollector();
    void publishAuthority();
    void rebuildPeerCombos();
    void updatePeerLabels();

    Config  config_;
    QString collectorUrl_;
    QString analyticsUrl_;
    QMap<QString, QString> collectors_;
    QMap<QString, QString> analytics_;
    std::unique_ptr<GitHubAuthorityStore> store_;

    QLabel*       status_ = nullptr;
    QLabel*       collectorLabel_ = nullptr;
    QLabel*       analyticsLabel_ = nullptr;
    QLabel*       next_ = nullptr;
    QComboBox*    collectorPick_ = nullptr;
    QComboBox*    analyticsPick_ = nullptr;
    QPushButton*  analyticsBtn_ = nullptr;
    QPushButton*  collectBtn_ = nullptr;
    QPushButton*  pushBtn_ = nullptr;
    QTableWidget* table_ = nullptr;
    QString       storePath_;
    bool          fillingPeers_ = false;
};
