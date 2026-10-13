# WORKSHOP EMPIRE – Projektfortschritt

## Aktuelle Phase

**Phase 6 – Reparatursystem: abgeschlossen, wartet auf Mikes Praxistest und Freigabe.**
Phase 7 wurde nicht begonnen.

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

### Phase 6 – Reparatursystem (2026-10-13)
- Reparieren an der Werkbank mit [Q]: Fehler suchen (Multimeter) → defektes Teil ausbauen → Ersatzteil einbauen → Funktionstest → optional aufbereiten
- Fehler sind pro Gerät verborgen und werden erst durch die Diagnose sichtbar; beschädigte Ersatzteile können den Funktionstest nicht bestehen
- Ersatzteile kommen aus dem Inventar oder dem Werkbanklager, gekauft oder aus einem anderen zerlegten Gerät
- Qualitätsverbesserung: Reinigungsset hebt die Qualität, ab 80 % wird ein Gerät "Sehr gut"
- Neue Gegenstände: Multimeter, Lötkolben, Kondensatoren-Set, Reinigungsset; neuer Händler "Ersatzteil-Express"
- Reparaturaufträge: Kunde bringt ein defektes Gerät (Lieferpalette), Rückgabe repariert in der Hand
- Kaputte Geräte sind deutlich weniger wert als funktionierende; gemessen pro PC: Reparieren +8 €, mit Aufbereiten +18 €, Zerlegen +6 €
- Speicherversion 5 (Reparaturstand, Reparaturaufträge)

## Offene Aufgaben

- Mike: Praxistest Phase 6 (Anleitung im Development Report)
- Mike: offene Produktentscheidungen in GAME_DESIGN.md §7 (u. a. persönliches Konto im Koop – bisher nur Firmenkonto umgesetzt)
- Später: Git LFS vor großen Assets; Remote-Repository für Backups
- Vorgemerkt: Sprint als `FSavedMove`-Flag; Physics Handle für schwere Objekte; Item-Icons; Spielerinventar nur an den Besitzer replizieren; Suchfeld für große Lager; Werkzeugtasche; Verkaufen direkt aus Werkbank-/Regallager; Reparatur weiterer Geräte (Motor, Haushaltsgeräte) mit eigenen Ersatzteilen

## Bekannte Fehler

Keine.

## Nächste Schritte (nach Freigabe)

Phase 7 – Erweiterte Beschaffung (Schrottplatz, Gebrauchtwarenhandel, Retourenpaletten, Auktionen, seltene Funde), erst nach Mikes Freigabe.

## Testergebnisse

Siehe `TEST_REPORT.md`. Letzter Lauf 2026-10-13: 53/53 Automation-Tests (inkl. Reparatur-Balance mit echten Daten), Netzwerktests A–L (inkl. Reparaturauftrag mit 2 Spielern) bestanden, Reparatur und Vertical Slice im Spiel mit Screenshots geprüft.
