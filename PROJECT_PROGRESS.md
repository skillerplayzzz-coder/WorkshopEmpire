# WORKSHOP EMPIRE – Projektfortschritt

## Aktuelle Phase

**Phase 2 – Items und Inventar: abgeschlossen, wartet auf Mikes Praxistest und Freigabe.**
Phase 3 wurde nicht begonnen.

## Abgeschlossen

### Phase 0 – Setup (2026-10-12)
- Umgebung geprüft (UE 5.7.4, VS 2026), C++-Projekt mit Modulen `WECore`, `WorkshopEmpire`, `WorkshopEmpireTests`
- Game Framework mit Koop-Limit 4, Geldtyp `FWEMoney`, Doku, Git

### Phase 1 – First-Person-Prototyp (2026-10-12)
- Bewegung (Gehen, Sprinten, Ducken, Springen), Interaktionssystem mit Hinweisen, Tragen/Ablegen/Untersuchen, Garagentor, Platzhalter-Garage
- Server-autoritativ mit Reichweitenprüfung und Konfliktschutz

### Phase 2 – Items und Inventar (2026-10-12)
- Datengetriebene Gegenstände: 9 Item-Definitionen als Data Assets (Computer, Röhrenmonitor, Schraubendreher, Werkzeugkiste, Elektromotor, Karton mit Kabeln, Blechschrauben, Kupferkabel, Arbeitsspeicher)
- Item-Instanzen mit Zustand (Neu … Schrott), Qualität, Menge, eindeutiger ID und Schätzwert
- Spielerinventar (20 Plätze, 25 kg) und Lager in Werkbank (8/60 kg) und Regal (15/150 kg)
- Stapelbare Materialien, Gewichts- und Platzgrenzen, Sortieren
- Inventarfenster (Tab/I) mit Drag & Drop, Umlagern, Ablegen in die Welt, Tooltips
- Speichern/Laden: F5/F9, benannte Slots, Autosave alle 5 min, Schutz gegen beschädigte Dateien (CRC + Backup)
- Alles server-autoritativ und für Koop getestet (keine Duplikate, private Spielerinventare)

## Offene Aufgaben

- Mike: Praxistest Phase 2 (Anleitung im Development Report)
- Mike: offene Produktentscheidungen in GAME_DESIGN.md §7
- Später: Git LFS vor großen Assets; Remote-Repository für Backups
- Vorgemerkt: Sprint als `FSavedMove`-Flag; Physics Handle für schwere Objekte; Item-Icons; Spielerinventar nur an den Besitzer replizieren; Suchfeld für große Lager (mit Lagerausbau in Phase 8)

## Bekannte Fehler

Keine.

## Nächste Schritte (nach Freigabe)

Phase 3 – Werkbank und Zerlegung: Gegenstand auf die Werkbank legen, wiederverwendbare Zerlegeabläufe als Daten (Schritte mit Werkzeuganforderung), Werkzeuge, Komponenten und Rohstoffe als Ergebnis, visuelles und akustisches Feedback. Ziel: Der alte Computer lässt sich in Komponenten zerlegen.

## Testergebnisse

Siehe `TEST_REPORT.md`. Letzter Lauf 2026-10-12: 26/26 Automation-Tests, Netzwerktests Phase 1 + 2 bestanden, Speichern/Laden im Spiel geprüft.
