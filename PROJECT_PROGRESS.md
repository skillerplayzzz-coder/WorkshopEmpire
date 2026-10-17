# WORKSHOP EMPIRE – Projektfortschritt

## Aktuelle Phase

**Phase 15 – Grafik und Atmosphäre: abgeschlossen, wartet auf Mikes Praxistest und Freigabe.**
Phase 16 wurde nicht begonnen.

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

### Phase 11 – Mitarbeiter und Logistik (2026-10-16)
- Mitarbeiter einstellen (Reiter "Personal & Logistik"): Lagerarbeiter, Maschinenbediener, Fahrer; 3 Bewerber, max. 6 Mitarbeiter, Erfahrung 1–3
- Aufgaben in der Werkstatt zuweisen ([E] auf Mitarbeiter, dann Quelle/Ziel anschauen); Pause, Entlassen, Materialfilter
- Lagertransporte (Lager/Maschine/Wagen → Lager/Maschine/Wagen), Maschinenbedienung (einschalten, warten, Produkte einlagern, +15 % Tempo)
- Löhne alle 5 Minuten; ohne Geld wird gestreikt, die Schuld bleibt
- Firmenwagen (Alter Transporter, Moderner Lieferwagen): Laderaum, Verkaufsfahrten (+15 %), Großeinkäufe; braucht einen Fahrer
- Gabelstapler: hebt bis 1.500 kg (neuer Großer Industriemotor 180 kg)
- Mitarbeiter laufen auf einem Navigationsnetz (passt sich der Einrichtung an)
- Speicherversion 10 (Mitarbeiter mit Aufgaben, Bewerber, Löhne, Touren)

### Phase 12 – Unternehmensexpansion (2026-10-16)
- Drei Gebäude zum Kaufen, die auf dem Gelände erscheinen (eigenes Tor, Licht, Baufläche): Werkstatt-Erweiterung (6.000 €), Gewerbehalle (25.000 €), Fabrikhalle (90.000 €); mehr Strom und Platz für Mitarbeiter
- Unternehmensstufen: Kleine Garage → Große Werkstatt → Gewerbehalle → Fabrik → Industrieunternehmen (Fabrik + 250.000 € Firmenwert); neue Stufe wird allen gemeldet
- Neue Produktion: Schmelzofen (Aluminium-/Kupferbarren), Verpackungsanlage (Paletten Stahlwinkel/Kabel); Hochregal und Großes Materiallager; Einrichtung erst ab der passenden Stufe
- Industrieaufträge (eigene Liste, größer, Vertragsstrafe bei Ablauf) und Lieferverträge mit mehreren Lieferungen; neue Wagentour "Auslieferung"
- Neuer Reiter "Unternehmen": Stufe, nächste Stufe, Firmenwert, Strom, Personal, Einnahmen und Ausgaben je Bereich (letzte Stunde / gesamt), laufende Aufträge
- Speicherversion 11 (Lieferungen von Aufträgen, Buchhaltung)

### Phase 13 – Multiplayer-Integration (2026-10-17)
- Hauptmenü (eigene Karte): neues Spiel oder Spielstand hosten, Spiel beitreten (Liste mit Host, Spielern, Ping, Version oder per IP), Name im LAN, Beenden
- Listen-Server mit Drop-in für 2–4 Spieler; Meldungen bei Beitritt und Verlassen
- Menü im Spiel [Esc]: Spielerliste mit Ping und Host, Entfernen (Host), Speichern (Host), Zum Hauptmenü (Host beendet für alle nach automatischem Speichern), Beenden
- Wer geht oder abstürzt, behält Position und Inventar beim Host (auch im Spielstand) und bekommt es beim Wiederkommen zurück
- Klare deutsche Meldungen bei Verbindungsabbruch, Zeitüberschreitung, vollem Spiel, anderer Version, Entfernen, Host beendet
- Versionsprüfung beim Beitritt; Speicherversion 12 (Spielernamen, gegangene Spieler)

### Phase 14 – Steam-Integration (2026-10-17)
- Steam-Sitzungen als Steam-Lobbys; Verbindung über Steam (keine Portfreigabe nötig)
- Freunde einladen über das Steam-Overlay, Beitreten über die Steam-Freundesliste, Steam-Spiele suchen
- Ohne Steam automatisch LAN/IP; Steam-Fehler werden als Meldung im Hauptmenü gezeigt
- Entwicklung mit Valves Test-App 480; eigene App ID für die Veröffentlichung nötig

### Phase 15 – Grafik und Atmosphäre (2026-10-17)
- Beleuchtung neu: Lumen (globale Beleuchtung und Reflexionen), Virtual Shadow Maps, physikalische Lichtwerte (Sonne, Leuchtstoffröhren), Wolken, Dunst mit Lichtstrahlen, Augenanpassung zwischen Hof und Werkstatt
- 51 kostenlose CC0-Modelle (Poly Haven) für Werkzeuge, Geräte, Regale, Kisten, Werkstattwagen, Rolltor, Hof-Deko; 14 echte Oberflächen (Ziegel, Beton, Putz, Wellblech, Asphalt, Metall, Holz); 108 CC0-Sounds (Kenney)
- Animierte Figuren für Spieler (Spielerfarbe) und Mitarbeiter (Rollenfarbe) mit Schrittgeräuschen
- Partikeleffekte: Staub, Späne, Funken, Rauch, Glut, Dampf, Schwebstaub; Maschinen mit Anlauf-/Lauf-/Auslaufgeräusch, Vibration und Effekten je Vorgang
- Rolltor rollt sich auf, Firmenwagen fährt sichtbar los und zurück, Hauptmenü-Kulisse mit echten Modellen
- Raumklang in der Werkstatt, draußen Wind und Vögel; Geräusche werden mit Entfernung leiser
- Grafikqualität im Haupt- und Spielmenü (Niedrig/Mittel/Hoch/Episch), Standard "Hoch": 71–83 fps in 1080p auf RTX 3060
- Lizenznachweis aller externen Assets in `Docs/ASSET_SOURCES.md`

## Offene Aufgaben

- Mike: Praxistest Phase 15 (Look und Klang, Leistung auf dem eigenen PC) und weiterhin Steam mit einem Freund (Phase 13/14)
- Mike: eigene Steam App ID vor einer Veröffentlichung (GAME_DESIGN.md §7)
- Mike: offene Produktentscheidungen in GAME_DESIGN.md §7 (u. a. persönliches Konto im Koop – bisher nur Firmenkonto umgesetzt)
- Git LFS ist nicht installiert; seit Phase 15 liegen ca. 760 MB Content im normalen Git. Vor einem Remote-Repository LFS einrichten
- Später: Remote-Repository für Backups
- Vorgemerkt: Sprint als `FSavedMove`-Flag; Physics Handle für schwere Objekte; Item-Icons; Spielerinventar nur an den Besitzer replizieren; Suchfeld für große Lager; Werkzeugtasche; Verkaufen direkt aus Werkbank-/Regallager; Reparatur weiterer Geräte (Motor, Haushaltsgeräte) mit eigenen Ersatzteilen; schwere Auktionsposten (Motoren) jetzt mit Sackkarre denkbar; Einrasten an Wänden im Baumodus; Stromausfall/Sicherung statt nur Warten; Förderbänder: Verteiler/Weiche, Bänder an Wänden entlang, Nachbarsuche cachen; Mitarbeiter: weitere Rollen (Reparaturtechniker, Produktionsarbeiter, Werkstattleiter), Mitarbeiter fahren den Gabelstapler, kleiner/großer LKW; Expansion: mehrere Standorte, Wagen sichtbar zum Hallentor, Balancing der Gebäudepreise (Phase 16); Grafik: eigene Maschinenmodelle statt lackierter Blöcke, Hängelampen in den Hallen, UI-Klickgeräusche und Aufprallgeräusche für Gegenstände (Kenney-Sounds sind schon importiert), Arbeitskleidung statt Mannequin-Optik

## Bekannte Fehler

Keine.

## Nächste Schritte (nach Freigabe)

Phase 16 – Balancing und Langzeitmotivation (laut Spezifikation), erst nach Mikes Freigabe.

## Testergebnisse

Siehe `TEST_REPORT.md`. Letzter Lauf 2026-10-17 (Phase 15): 103/103 Automation-Tests, Grafik-Screenshots aller Ansichten geprüft, Leistung je Qualitätsstufe gemessen, Regression Netzwerktests A–X, I/J, 4 Spieler über das Hauptmenü, Spielläufe P5–P14.
