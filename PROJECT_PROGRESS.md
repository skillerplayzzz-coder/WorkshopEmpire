# WORKSHOP EMPIRE – Projektfortschritt

## Aktuelle Phase

**Phase 5 – Erster vollständiger Gameplay-Loop (Vertical Slice): abgeschlossen, wartet auf Mikes Praxistest und Freigabe.**
Phase 6 wurde nicht begonnen.

## Abgeschlossen

### Phase 0 – Setup (2026-10-12)
- Umgebung geprüft (UE 5.7.4, VS 2026), C++-Projekt mit Modulen `WECore`, `WorkshopEmpire`, `WorkshopEmpireTests`, Doku, Git

### Phase 1 – First-Person-Prototyp (2026-10-12)
- Bewegung, Interaktion, Tragen/Ablegen/Untersuchen, Garagentor, Platzhalter-Garage; server-autoritativ

### Phase 2 – Items und Inventar (2026-10-12)
- Datengetriebene Gegenstände, Inventar mit Drag & Drop, Lager, Speichern/Laden mit Schutz gegen beschädigte Dateien

### Phase 3 – Werkbank und Zerlegung (2026-10-12)
- Werkbank mit schrittweisem Zerlegen, Werkzeuge, Komponenten und Rohstoffe, Feedback, Sounds

### Phase 4 – Wirtschaft und Handel (2026-10-12)
- Firmenkonto: 250 € Startkapital, Cent-genau, nur der Server bucht, Schutz vor Doppelbuchungen
- Markt mit schwankender Nachfrage je Kategorie; Preise aus Wert, Zustand, Qualität, Nachfrage und Händler
- Online-Marktplatz-Terminal in der Garage mit 3 Händlern (Schrotthandel, Gebrauchtbörse, Werkzeug-Versand)
- Verkaufen (aus dem Inventar oder den getragenen Gegenstand), Kaufen (Kleinteile ins Inventar, Großes auf die Lieferpalette)
- Kundenaufträge: Angebote, Annehmen, Abliefern, Fristen, Belohnung aufs Firmenkonto
- Kontoanzeige im HUD, Handelsbildschirm mit letzten Buchungen
- Balancing: defekter PC kaufen → zerlegen → verkaufen bringt im Schnitt ca. 6 € Gewinn
- Speicherversion 3 (Konto, Markt, Aufträge, Vorrat)

### Phase 5 – Vertical Slice (2026-10-13)
- Kompletter Ablauf spielbar: Werkzeug und Gerät besorgen → zerlegen → "Alles nehmen" → Teile verkaufen → in die Werkstatt investieren → speichern
- 6 Werkstatt-Upgrades (Daten-Asset): LED-Beleuchtung, Werkbank-Unterschrank, zweites Regal, Werkzeuggürtel, Arbeitsmatte, Profi-Werkbank; teils mit Voraussetzung
- Wirkungen: mehr Werkbankplätze, mehr Tragkraft für alle, schnelleres Zerlegen, hellere Lampen, neues Möbelstück erscheint
- Neue Spalte "Werkstatt verbessern" am Marktplatz; Kauf nur über den Server, gilt für das ganze Team
- Leitfaden "Erste Schritte (1/6)" oben links, rückt nur bei echten Aktionen vor; danach "Nächstes Ziel"
- "Alles nehmen"-Knopf in Lagern
- Speicherversion 4 (Upgrades, Leitfaden)

## Offene Aufgaben

- Mike: Praxistest Phase 5 (Anleitung im Development Report)
- Mike: offene Produktentscheidungen in GAME_DESIGN.md §7 (u. a. persönliches Konto im Koop – bisher nur Firmenkonto umgesetzt)
- Später: Git LFS vor großen Assets; Remote-Repository für Backups
- Vorgemerkt: Sprint als `FSavedMove`-Flag; Physics Handle für schwere Objekte; Item-Icons; Spielerinventar nur an den Besitzer replizieren; Suchfeld für große Lager; Werkzeugtasche; Verkaufen direkt aus Werkbank-/Regallager

## Bekannte Fehler

Keine.

## Nächste Schritte (nach Freigabe)

Phase 6 laut Spezifikation (MASTER_PROMPT), erst nach Mikes Freigabe.

## Testergebnisse

Siehe `TEST_REPORT.md`. Letzter Lauf 2026-10-13: 46/46 Automation-Tests (inkl. kompletter Loop mit echten Daten), Koop-Test als Listen-Server (I/J), Netzwerktests A–H bestanden, kompletter Ablauf im Spiel mit Screenshots geprüft.
