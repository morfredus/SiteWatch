/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QByteArray>
#include <QEventLoop>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRunnable>
#include <QThreadPool>
#include <QTimer>
#include <functional>

// -----------------------------------------------------------------------------
// Aides reseau pour les dialogues : aucune ne laisse la fenetre figee ni ne
// laisse l'utilisateur relancer une action pendant l'attente.
// -----------------------------------------------------------------------------
namespace netpump {

// Execute `work` dans le pool de threads et rend la main quand c'est fini. L'attente se fait
// dans une boucle d'evenements qui ignore clics et frappes : la fenetre se redessine (curseur
// d'attente, texte d'etat) sans reentrance possible. `work` ne doit toucher aucun widget.
inline void runPumped(const std::function<void()>& work) {
    QEventLoop loop;
    QThreadPool::globalInstance()->start(QRunnable::create([&loop, &work] {
        work();
        QMetaObject::invokeMethod(&loop, [&loop] { loop.quit(); }, Qt::QueuedConnection);
    }));
    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

// GET synchrone borne par un delai : sans lui, un serveur qui ne repond pas bloque
// l'appelant indefiniment. Renvoie l'erreur reseau (TimeoutError si delai depasse).
inline QNetworkReply::NetworkError boundedGet(QNetworkAccessManager& nam,
                                              const QNetworkRequest& req, QByteArray& body,
                                              int timeoutMs = 15000) {
    QNetworkReply* r = nam.get(req);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(r, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    QNetworkReply::NetworkError err = QNetworkReply::TimeoutError;
    if (r->isFinished()) {
        body = r->readAll();
        err = r->error();
    } else {
        r->abort();   // delai depasse
    }
    r->deleteLater();
    return err;
}

// Pause sans bloquer le dessin, sans accepter de saisie.
inline void pause(int ms) {
    QEventLoop wait;
    QTimer::singleShot(ms, &wait, &QEventLoop::quit);
    wait.exec(QEventLoop::ExcludeUserInputEvents);
}

} // namespace netpump
