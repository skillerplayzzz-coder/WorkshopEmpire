# WORKSHOP EMPIRE – Projektfortschritt

## Aktuelle Phase

**Phase 3 – Werkbank und Zerlegung: abgeschlossen, wartet auf Mikes Praxistest und Freigabe.**
Phase 4 wurde nicht begonnen.

## Abgeschlossen

### Phase 0 – Setup (2026-10-12)
- Umgebung geprüft (UE 5.7.4, VS 2026), C++-Projekt mit Modulen `WECore`, `WorkshopEmpire`, `WorkshopEmpireTests`
- Game Framework mit Koop-Limit 4, Geldtyp `FWEMoney`, Doku, Git

### Phase 1 – First-Person-Prototyp (2026-10-12)
- Bewegung, Interaktionssystem mit Hinweisen, Tragen/Ablegen/Untersuchen, Garagentor, Platzhalter-Garage
- Server-autoritativ mit Reichweitenprüfung und Konfliktschutz

### Phase 2 – Items und Inventar (2026-10-12)
- Datengetriebene Gegenstände (Data Assets), Item-Instanzen mit Zustand/Qualität/Menge/ID
- Spielerinventar, Werkbank- und Regallager, Stapeln, Inventarfenster mit Drag & Drop
- Speichern/Laden mit Slots, Autosave und Schutz gegen beschädigte Dateien

### Phase 3 – Werkbank und Zerlegung (2026-10-12)
- Werkbank: sperrigen Gegenstand tragen und mit [E] auf die Werkbank legen, mit [R] wieder herunternehmen
- Wiederverwendbare Zerlegeabläufe als Data Assets: Alter Computer (7 Schritte), Röhrenmonitor (4), Karton mit Kabeln (1)
- Werkzeuge: Schraubendreher, Seitenschneider (in der Garage), Akkuschrauber (definiert, ×2,5 schneller, noch nicht in der Welt)
- Ergebnisse: Komponenten (Grafikkarte, Arbeitsspeicher, Netzteil, Mainboard) mit vererbtem Zustand und Rohstoffe (Stahlschrott, Kunststoffteile, Kupferkabel, Glasbruch, Elektronikschrott, Schrauben)
- Feedback: Fortschrittsbalken, Leiste an der Werkbank, Meldung mit den gewonnenen Teilen, 5 selbst erzeugte Sounds
- Koop: ein Arbeitsschritt gleichzeitig, wer weggeht bekommt nichts, beide Spieler sehen dieselbe Werkbank
- Speichern: Zerlegefortschritt und Gegenstand auf der Werkbank (Speicherversion 2)

## Offene Aufgaben

- Mike: Praxistest Phase 3 (Anleitung im Development Report), Sounds anhören
- Mike: offene Produktentscheidungen in GAME_DESIGN.md §7
- Später: Git LFS vor großen Assets; Remote-Repository für Backups
- Vorgemerkt: Sprint als `FSavedMove`-Flag; Physics Handle für schwere Objekte; Item-Icons; Spielerinventar nur an den Besitzer replizieren; Suchfeld für große Lager (Phase 8); kleine Gegenstände aus dem Inventar auf die Werkbank legen; Werkzeugtasche

## Bekannte Fehler

Keine.

## Nächste Schritte (nach Freigabe)

Phase 4 – Wirtschaft und Handel: Firmenkonto (250 € Startkapital, `FWEMoney`, serverseitig), Händler mit Kauf- und Verkaufspreisen (abhängig von Zustand, Qualität, Nachfrage), Gegenstände verkaufen und kaufen, erste Kundenaufträge, Transaktionsprüfungen gegen Doppelbuchungen.

## Testergebnisse

Siehe `TEST_REPORT.md`. Letzter Lauf 2026-10-12: 33/33 Automation-Tests, Netzwerktests Phase 1–3 bestanden, Zerlegung im Spiel mit Screenshots geprüft.
