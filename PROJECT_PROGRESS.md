# WORKSHOP EMPIRE – Projektfortschritt

## Aktuelle Phase

**Phase 1 – Spielbarer First-Person-Prototyp: abgeschlossen, wartet auf Mikes Praxistest und Freigabe.**
Phase 2 wurde nicht begonnen.

## Abgeschlossen

### Phase 0 – Setup (2026-10-12)
- Umgebung geprüft (UE 5.7.4, VS 2026), C++-Projekt mit Modulen `WECore`, `WorkshopEmpire`, `WorkshopEmpireTests`
- Game Framework mit Koop-Limit 4, Geldtyp `FWEMoney`, Doku, Git

### Phase 1 – First-Person-Prototyp (2026-10-12)
- First-Person-Charakter: Gehen, Sprinten (Shift), Ducken (Strg/C, weiche Kamera), Springen, Mausblick mit einstellbarer Empfindlichkeit (`we.MouseSensitivity`, `we.InvertY`)
- Interaktionssystem: Schnittstelle `IWEInteractable`, Fokus-Trace, Hinweise "[E] …" / "[F] Untersuchen", Tastennamen aus der Belegung
- Gegenstände aufnehmen, tragen, ablegen (auf Werkbank/Regal/Boden, wo man hinschaut), untersuchen; Gewichtslimit 30 kg
- Garagentor zum Öffnen/Schließen
- Platzhalter-Garage `L_Garage` mit Werkbank, Regal und 6 Gegenständen, per Skript reproduzierbar
- Server-autoritativ: Reichweitenprüfung, Konfliktschutz (zwei Spieler, ein Gegenstand), Ablegen beim Verlassen
- 14 Automation-Tests, Netzwerk-Rauchtest mit 2 Clients, automatisierte Screenshots

## Offene Aufgaben

- Mike: Praxistest Phase 1 (Anleitung im Development Report)
- Mike: offene Produktentscheidungen in GAME_DESIGN.md §7
- Später: Git LFS vor großen Assets; Remote-Repository für Backups
- Vorgemerkt: Sprint als `FSavedMove`-Flag, falls Positionskorrekturen spürbar sind; Physics Handle für schwere Objekte

## Bekannte Fehler

Keine.

## Nächste Schritte (nach Freigabe)

Phase 2 – Items und Inventar: Item-Definitionen als Data Assets, Item-Instanzen (Zustand, Qualität), Inventarkomponente (repliziert), stapelbare Materialien, Inventaroberfläche, einfaches Speichern/Laden. `AWEPickupItem` wird zur Weltdarstellung einer Item-Instanz.

## Testergebnisse

Siehe `TEST_REPORT.md`. Letzter Lauf 2026-10-12: 14/14 Automation-Tests bestanden, Netzwerk-Rauchtest bestanden, 8 Screenshots geprüft.
