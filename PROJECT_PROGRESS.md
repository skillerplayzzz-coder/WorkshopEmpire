# WORKSHOP EMPIRE – Projektfortschritt

## Aktuelle Phase

**Phase 4 – Wirtschaft und Handel: abgeschlossen, wartet auf Mikes Praxistest und Freigabe.**
Phase 5 wurde nicht begonnen.

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

## Offene Aufgaben

- Mike: Praxistest Phase 4 (Anleitung im Development Report)
- Mike: offene Produktentscheidungen in GAME_DESIGN.md §7 (u. a. persönliches Konto im Koop – bisher nur Firmenkonto umgesetzt)
- Später: Git LFS vor großen Assets; Remote-Repository für Backups
- Vorgemerkt: Sprint als `FSavedMove`-Flag; Physics Handle für schwere Objekte; Item-Icons; Spielerinventar nur an den Besitzer replizieren; Suchfeld für große Lager; Werkzeugtasche; Verkaufen direkt aus Werkbank-/Regallager

## Bekannte Fehler

Keine.

## Nächste Schritte (nach Freigabe)

Phase 5 – Erster vollständiger Gameplay-Loop (Vertical Slice): beschaffen → zerlegen → verkaufen → Werkzeuge kaufen → Werkstatt verbessern → Fortschritt speichern, als zusammenhängender, wiederholbarer Ablauf mit erstem Werkstatt-Upgrade und Spielstart-Ablauf. Laut Spezifikation besonders gründlich zu testen.

## Testergebnisse

Siehe `TEST_REPORT.md`. Letzter Lauf 2026-10-12: 41/41 Automation-Tests, Netzwerktests A/B, C/D, E/F, G/H bestanden, Handel im Spiel mit Screenshots geprüft.
