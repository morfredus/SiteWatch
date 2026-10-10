/*
 * SiteWatch
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Auto-test du magasin d'autorite web : consolidation, detection de changement
 * par taille (y compris une taille plus PETITE apres rotation) et remplacement
 * des jours d'un site sans toucher aux autres.
 *
 *   sitewatch-webstore-selftest        # code retour 0/1
 */

#include "webstore/WebAuthorityStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>

#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* what) {
    std::cout << (ok ? "  OK  " : "  KO  ") << what << '\n';
    if (!ok) ++failures;
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    WebAuthorityStore store(dir.filePath(QStringLiteral("web.sqlite")));
    check(store.open(), "ouverture de la base");

    Stats s;
    s.dailyHumans = {{"2026-10-01", 10}, {"2026-10-02", 20}};
    s.dailyBots = {{"2026-10-01", 5}};
    QMap<QString, qint64> files{{"a.gz", 100}, {"b.gz", 200}};

    check(store.needsConsolidation("A", files), "site jamais consolide : a consolider");
    store.consolidate("A", s, files);
    check(!store.needsConsolidation("A", files), "memes fichiers : rien a faire");
    check(store.lastDay("A") == "2026-10-02", "dernier jour memorise");
    check(store.daily("A").value("daily_humans").toObject().value("2026-10-02").toInt() == 20,
          "serie quotidienne relue");
    check(store.consolidatedAt("A") > 0, "date de consolidation renseignee");

    files["a.gz"] = 150;
    check(store.needsConsolidation("A", files), "fichier qui grossit : a reconsolider");
    files["a.gz"] = 50;
    check(store.needsConsolidation("A", files), "fichier plus petit (rotation) : a reconsolider");
    files.remove("a.gz");
    files["c.gz"] = 10;
    check(store.needsConsolidation("A", files), "fichier nouveau : a reconsolider");

    // Reconsolider remplace les jours du site, et laisse les autres sites intacts.
    store.consolidate("B", s, {{"x.gz", 1}});
    Stats s2;
    s2.dailyHumans = {{"2026-10-03", 7}};
    store.consolidate("A", s2, files);
    check(store.daily("A").value("daily_humans").toObject().size() == 1, "jours du site remplaces");
    check(store.daily("B").value("daily_humans").toObject().size() == 2, "autre site intact");

    std::cout << (failures ? "ECHEC\n" : "Tout est conforme.\n");
    return failures ? 1 : 0;
}
