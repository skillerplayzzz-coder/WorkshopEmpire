# WORKSHOP EMPIRE – Projektfortschritt

## Aktuelle Phase

**Phase 10 – Produktion und Automatisierung: abgeschlossen, wartet auf Mikes Praxistest und Freigabe.**
Phase 11 wurde nicht begonnen.

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

### Phase 7 – Erweiterte Beschaffung (2026-10-14)
- Schrotthaufen hinter dem Tor: bis zu 4 Gratis-Teile (Schrott, kaputte Geräte, selten etwas Wertvolles), Nachschub alle 8 Minuten
- Gebrauchtwaren mit wechselndem Angebot: bei jedem Nachschub wird gewürfelt, was da ist und in welchem Zustand
- Retourenpaletten (Elektronik, Werkzeug) mit unbekanntem Inhalt, an der Lieferpalette mit [Q] öffnen
- Auktionen (Geschäftsauflösungen, Industrie, Nachlässe) im neuen Reiter "Auktionen & Beschaffung": ein Mitbieter mit geheimem Limit, bezahlt wird erst beim Zuschlag, Ware kommt auf die Palette
- Seltene Gegenstände: Grafikkarte (Sammlerstück), Röhrenradio (1958), Keramik-Prozessor mit Goldkontakten, Industrie-Servomotor
- Erweiterte Kundenaufträge: Mindestqualität, große Geräte in der Hand abliefern, Eilaufträge, Sammleraufträge
- Speicherversion 6 (gewürfelter Händlervorrat, laufende Auktionen, Auftragsqualität)

### Phase 8 – Werkstatt-Upgrades (2026-10-14)
- Baumodus [B]: Vorschau (grün/rot), Kollisionsprüfung, Drehen ([R] 90°, [Q] 15°), Einrasten im 10-cm-Raster, Bestätigen [E], Verschieben, Abbauen [X] (halber Preis zurück), Speichern
- Einrichtung zum Kaufen: Werkbank, Große Werkbank (20 Plätze, 20 % schneller), Schwerlastregal, Großes Lagerregal (25 Plätze, 300 kg), Werkzeugwand, Lagerkiste, Ablagetisch
- Mehrere Werkbänke gleichzeitig nutzbar; neue Werkbänke bekommen auch den Werkbank-Unterschrank
- Neue Werkzeuge: Bit-Ratsche, Profi-Seitenschneider, Digitalmultimeter, Lötstation; Sackkarre (+30 kg Tragkraft, damit lässt sich der Elektromotor bewegen)
- Speicherversion 7 (Einrichtung und Positionen)

### Phase 9 – Erste Industriemaschinen (2026-10-15)
- Schredder (450 €, 7,5 kW), Sortiermaschine (380 €, 4 kW), Metallpresse (320 €, 5,5 kW), aufgestellt über den Baumodus
- Erste Verarbeitungskette: Gerät/Schrott → Schreddergut → Kupfer, Aluminium, Stahl, Kunststoff, Elektronikschrott → Stahl-/Aluminiumballen, Kupferpaket
- Bedienung: [E] Eingang bzw. getragenes Gerät hinein, [Q] ein/aus, [R] Produkte entnehmen, [X] warten, [F] Status; Status mit Fortschritt im HUD
- Zustände: Aus, Bereit, Läuft, Ausgabe voll, Stromanschluss überlastet, Wartung nötig; Ausschalten pausiert
- Strom: 11 kW Hausanschluss, Upgrade Starkstromanschluss (+11 kW, 300 €), Stromrechnung alle 5 Minuten, Anzeige "Strom x / y kW"
- Verschleiß und Wartung (Wartungsset), 8 neue Gegenstände, Rezepte als Daten
- Speicherversion 8 (Maschinen inkl. laufendem Vorgang, Eingang und Ausgabe getrennt, Stromzähler)

### Phase 10 – Produktion und Automatisierung (2026-10-16)
- Förderbänder (1 m, 2 m) im Baumodus: nehmen Produkte aus der Maschine bzw. dem Materiallager dahinter, geben vorn an Band, Maschine oder Lager ab; über Eck verbindbar, stauen sich bei vollem Ziel
- Maschinenverbindungen und Materialfluss: eine Linie wie Zuführband → Schredder → Sortiermaschine → Materiallager → Fertigungsautomat läuft ohne Spieler
- Produktionsrezepte wählbar mit [T] (Automatisch oder fest); zweite Zutat möglich
- Fertigungsautomat (600 €, 6 kW): Kupferdrahtspule, Verlängerungskabel, Stahlwinkel-Set; Produkte kauft die Gebrauchtbörse, 4 neue Produktionsaufträge
- Materiallager (Produktionslager): Ausgabe aufs Band mit [T] (Alles / ein Material / Aus), schickt nur, was die Maschine am Ende der Linie annimmt
- Bänder: Gegenstände auflegen/herunternehmen [E], anhalten [Q], untersuchen [F]
- Untersuchen-Fenster wächst mit dem Inhalt
- Speicherversion 9 (Bänder mit Gegenständen und Positionen, Lagerausgabe, gewähltes Rezept)

## Offene Aufgaben

- Mike: Praxistest Phase 10 (Anleitung im Development Report)
- Mike: offene Produktentscheidungen in GAME_DESIGN.md §7 (u. a. persönliches Konto im Koop – bisher nur Firmenkonto umgesetzt)
- Später: Git LFS vor großen Assets; Remote-Repository für Backups
- Vorgemerkt: Sprint als `FSavedMove`-Flag; Physics Handle für schwere Objekte; Item-Icons; Spielerinventar nur an den Besitzer replizieren; Suchfeld für große Lager; Werkzeugtasche; Verkaufen direkt aus Werkbank-/Regallager; Reparatur weiterer Geräte (Motor, Haushaltsgeräte) mit eigenen Ersatzteilen; schwere Auktionsposten (Motoren) jetzt mit Sackkarre denkbar; Einrasten an Wänden im Baumodus; Maschinengeräusche, Stromausfall/Sicherung statt nur Warten; Förderbänder: Verteiler/Weiche, Bänder an Wänden entlang, Nachbarsuche cachen

## Bekannte Fehler

Keine.

## Nächste Schritte (nach Freigabe)

Phase 11 – Mitarbeiter und Logistik (laut Spezifikation u. a. Mitarbeiter einstellen, Aufgaben zuweisen), erst nach Mikes Freigabe.

## Testergebnisse

Siehe `TEST_REPORT.md`. Letzter Lauf 2026-10-15: 75/75 Automation-Tests, Netzwerktests A–R (inkl. Maschinen mit 2 Spielern) bestanden, Maschinenkette, Baumodus, Beschaffung, Reparatur und Vertical Slice im Spiel mit Screenshots geprüft.
