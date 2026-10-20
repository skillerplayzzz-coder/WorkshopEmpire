# WORKSHOP EMPIRE – Projektfortschritt

## Aktuelle Phase

**Phase 19 – Verbesserungen nach Mikes Auswahl (0.19.0): abgeschlossen, wartet auf Mikes Praxistest.**

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

### Phase 16 – Balancing und Langzeitmotivation (2026-10-17)
- Langzeit-Playtest als Simulation auf den echten Spieldaten (60 h allein und zu zweit, automatischer Test mit Zielfenstern); Methode und Ergebnis in `Docs/BALANCING.md`
- Vorher blieb das Einkommen ab Stunde 6 bei ~900 €/h stehen, Fabrik und Endziel waren unerreichbar; jetzt allein: Große Werkstatt 10 h, Gewerbehalle 18 h, Fabrik 32 h, Industrieunternehmen 45 h
- Größere Abholtouren je Unternehmensstufe (Elektroschrott-Container, Kabelschrott, Großauflösung Bürogebäude, Fabrikauflösung), Lkw mit Ladekran
- Stärkere Maschinenketten (mehr Kupfer/Aluminium aus Elektronik), neue Endgame-Maschine Edelmetall-Rückgewinnung in der Fabrik
- 14 neue Aufträge (Sammleraufträge für seltene Funde, Schulausstattung, Lieferverträge, Großprojekte ab Stufe 5)
- 31 Meilensteine mit Prämien bis zum Firmenwert von 1 Mio. € (Reiter "Unternehmen"), Speicherversion 13
- Behoben: Seit Phase 15 stand der Firmenwagen in der Garage statt vor dem Tor

### Phase 17 – Qualitätssicherung (2026-10-18)
- Leistungsprofil einer vollen Fabrik (28 Maschinen, 20 Mitarbeiter, 400 Gegenstände): 54–68 fps in 1080p; Speichern/Laden 11/134 ms
- Multiplayer-Stress und Dauerlauf: 4 Spieler 30 min mit Lag, Paketverlust, Umordnung – 14.400 Aktionen, 0 Fehler, Konto immer gleich
- Speichertests: 131 beschädigte Dateien sauber abgelehnt, 10× speichern/laden identisch, alte Versionen laden, 2.000 Gegenstände in 0,5 s
- UI- und Eingabetest in 4 Auflösungen über den echten Eingabeweg; Dauerlauf 30 min ohne Speicherwachstum
- Behoben: Gegenstände konnten durch den Hofboden fallen; 4 Textüberläufe in Menüs; fehlende Includes; Testwerkzeuge (Teleport, Bildrate)
- Werkzeuge für spätere Prüfungen: `Docs/QA.md`

### Phase 17.5 – Visual Overhaul, Gameplay Polish & Immersion (2026-10-18)
- Neue Oberfläche in einem Industrie-Stil (HUD, Inventar mit Bildern und Kontextmenü, Hauptmenü, Pausemenü, Einstellungen mit Auflösung, Fenstermodus, UI-Skalierung, Maus, Sichtfeld, Lautstärke); 65 eigene Icons, Item-Bilder aus den Modellen gerendert
- Werkstatt-PC am Schreibtisch mit eigenem Betriebssystem „Ferrit OS“ und 8 Programmen statt des Marktterminals
- Arbeitsansicht an der Werkbank: Zerlegen und Reparieren von Hand mit Werkzeugwahl, Schrauben, Klammern, Steckern, Abdeckung, Teilen, Messpunkten, Funktionstest; vom Server geprüft
- Industriegebiet: Straße, Holzmasten mit Leitungen, Straßenlampen, Maschendrahtzaun mit Tor, Nachbargebäude, Container, Kram; wächst mit der Firma; auch hinter dem Hauptmenü
- Werkstatt gestalten (Wandfarbe, Boden, Licht, Tor pro Gebäude) und 13 Deko-Möbel; Verkaufsschutz (Ausrüstung, Favorit, Sperren); alte Gebäude verkaufen (35 %); Tutorial mit 11 Schritten; neue Transporter-Modelle
- Spielbarer Build (Development) gepackt und getestet; 116/116 Tests; Regression grün
- Behoben: Absturz im gepackten Spiel (Firmenstufen-Text), fehlende Inhalte im Build, durchsichtiges Rolltor, Zaun unsichtbar im Build

### Phase 18 – PC-System, Transporter, Charaktereditor, UI und Stadt (2026-10-18, auf Mikes Wunsch)
- Alter PC als echte Baugruppe: Gehäuse, Seitenteil, Mainboard, Grafikkarte, 2× RAM, CPU-Kühler, Prozessor, HDD, SSD, Gehäuselüfter, Netzteil, CD-Laufwerk an echten Einbauplätzen; jedes Teil mit eigenen Schrauben, Steckern, Clips oder Hebel; Bauteil anklicken, Befestigungen lösen, Teil wird herausgezogen; freie Reihenfolge nach Voraussetzungen; Orbit-Kamera, Hervorhebung; alle Teile und Schrauben als echte Gegenstände; neue Gegenstände CPU-Kühler, Prozessor, Festplatte, SSD, Gehäuselüfter; Speicherstand Version 15
- Transporter neu: US-Kastenwagen mit langer Haube, Kühlergrill, Scheinwerfern, Felgen, Spiegeln, Griffen, Innenraum (Sitze, Armaturenbrett, Lenkrad), Laderaum mit Regal; Fahrer-, Beifahrer- und Hecktüren mit Animation und Ton; Ladung im Laderaum sichtbar
- Charaktereditor beim neuen Spiel (und im Multiplayer-Menü): Mann/Frau, Gesicht, 5 Hauttöne, 6 Frisuren, 6 Haarfarben, 5 Outfits in 5 Farben, 3 Schuhe, Cap/Mütze, Sonnenbrille; 3D-Vorschau zum Drehen; im Profil und im Spielstand; Mitspieler sehen die Figur, [V] zeigt die eigene von hinten
- Baumenü mit 5 Kategorien, Bildern, Preisen und Kurzbeschreibung; Vorschau zeigt das echte Modell durchsichtig grün/rot; Tutorial-Karte kleiner
- Stadt: zwei Kreuzungen mit Ampeln und Zebrastreifen, Tankstelle, Parkplatz, Autowerkstatt, Imbiss, Brachfläche mit Plakatwand, geparkte Autos, Pflanzen; Straßen laufen weiter, Baustellen sperren sie am Rand des Spielbereichs; Gebäude dahinter, Skyline
- Behoben: Mitarbeiter fanden den Weg aus der Garage nicht (seit 17.5)

### Nachbesserungen nach Phase 18 (0.18.1, 2026-10-18, auf Mikes Wunsch)
- PC wieder zusammenbauen: Arbeitsansicht mit dritter Ansicht „Zusammenbauen“; ausgebautes Teil in der Liste wählen, es schwebt vor seinem Platz, wird eingesetzt, dann Stecker, Klammern und Schrauben in umgekehrter Reihenfolge; Teile und Schrauben kommen aus Inventar oder Werkbanklager (beste zuerst); Reihenfolge prüft, was davor ausgebaut und was darunter eingebaut sein muss; CD-Laufwerk (wird beim Ausbau verwertet) und Gehäuselüfter sind optional
- Funktionstest danach („Testen/Reparieren“): läuft nur, wenn alles Nötige drin ist; mit guten Teilen läuft der PC (Zustand = schlechtestes Teil), mit defekten Teilen schlägt er fehl und nennt sie; ungetestet verkauft er sich wie ein defekter; eingebaute Teile kommen beim erneuten Ausbau so heraus, wie sie hineingingen; Speicherstand Version 16
- Werkbanklager 24 statt 12 Plätze (100 kg), große Werkbank 36 Plätze
- Transporter: jede Ladung wird einzeln von den Händen durch die Hecktüren an ihren Platz getragen; mehrere Stapel nacheinander; beim Ausladen fliegt jedes sperrige Teil einzeln heraus und erscheint erst dann hinter dem Wagen
- Sounds: alle selbst erzeugten Sounds ersetzt (15 Pixabay-Aufnahmen, 5 Kenney-Sounds, Quellen in `Docs/ASSET_SOURCES.md`)
- Geparkte Autos: Kenney Car Kit (CC0, ohne Marken) statt eigener Klötzchen-Modelle

### Phase 19 – Verbesserungen nach Mikes Auswahl (0.19.0, 2026-10-19)
- PC-Aufträge: PCs bauen oder aufrüsten mit Mindest-Benchmark; bessere Teile (Grafikkarte Pro, 8-GB-Riegel, schneller Prozessor, schnelle SSD) passen in dieselben Plätze; Sterne-Bewertung mit Bonus
- Kleinanzeigen im Ferrit OS: von Privatleuten kaufen (handeln), eigene Sachen inserieren
- Tag und Nacht mit Öffnungszeiten, Mondlicht und Straßenlaternen; Laufkundschaft am Tor (Reparatur, Ankauf, Verkauf)
- Figur: echte Kleidungsmodelle (12 je Körper), 8 Männer- und 5 Frauengesichter, Spiegel in der Werkstatt zum Umziehen
- Komfort: Suchen und Sortieren in Lagern, Verkaufen direkt aus den Werkstattlagern, Werkzeugtasche
- Gabelstapler als echtes Modell, weg aus der Ecke
- Oberfläche: Schatten, runde Ecken, Sonne/Mond-Uhr, Spielzeit im Ferrit OS, deutsche Zahlen, kürzere Namen in Inventarfeldern
- Sicherung: Git LFS für große Dateien, lokales Backup-Repository `G:\Backups\WorkshopEmpire.git`

### Phase 20 – Werkstatt-Ausbau und Release-Vorbereitung (0.20.0, 2026-10-20)
- Firmenlogo-Editor im Ferrit OS (Symbol, Farben, Arbeitskleidung, Name); Logo über dem Tor, am Firmenschild, auf dem Transporter und auf dem Overall
- Mitarbeiter als echte Figuren (Körper, Gesichter, Kleidung wie Spieler und Kunden) im Overall in der Firmenfarbe
- Reparatur von Fernseher, Kofferradio, Spielkonsole und Akkuschrauber mit eigenen Fehlern und Ersatzteilen
- Atmosphäre: Werkstattradio mit 6 Funk-Titeln, Kaffeemaschine (3 min schneller), Werkstattkatze „Schraube“, Wetter mit Regen und Gewitter
- Einstellungen: Schieberegler, Bewegungsunschärfe, frei belegbare Tasten
- Release: Shipping-Build, Steam-App-ID-Skript, Steam-Cloud-Pfade, Upload-Vorlagen, Trailer aus Spielszenen (`Docs/RELEASE_STEAM.md`)
## Offene Aufgaben

- Mike: Steamworks-Konto anlegen und die eigene App ID eintragen (`Tools/Release/set_steam_appid.ps1`), Auto-Cloud im Backend einstellen; Cloud-Sync ist erst damit testbar
- Mike: Praxistest Phase 20 (Logo-Editor, Mitarbeiter, neue Reparaturen, Radio, Kaffee, Katze, Wetter, Tastenbelegung, Trailer ansehen)

- Mike: Praxistest Phase 18 und 0.18.1 (PC zerlegen und wieder zusammenbauen, Funktionstest, Transporter beladen, neue Sounds anhören, Autos)
- Mike: Praxistest Phase 19 (PC-Aufträge, Kleinanzeigen, Tag/Nacht, Laufkundschaft, Kleidung und Spiegel)
- Mike: für ein Online-Backup ein GitHub-Konto bzw. privates Repository nennen; Git LFS auch für Visual Studio installieren (git-lfs.com), damit VS große Dateien pushen kann
- Sounds: ausgewählt nach Titel, Länge und Pegel; im Spiel noch nicht angehört (kein Audio im Testlauf). Was nicht passt, wird auf Hinweis getauscht

- Mike: Praxistest Phase 17.5 (Arbeitsansicht – fühlt sich das Schrauben gut an? Ferrit OS, Gestaltung, Tutorial, Koop an einer Werkbank)
- Mike: Praxistest Phase 16 (fühlt sich das Tempo richtig an? die Simulation ersetzt keinen echten Spieler) und weiterhin Phase 15 (Klang) sowie Steam mit einem Freund
- Mike: eigene Steam App ID vor einer Veröffentlichung (GAME_DESIGN.md §7)
- Mike: offene Produktentscheidungen in GAME_DESIGN.md §7 (u. a. persönliches Konto im Koop – bisher nur Firmenkonto umgesetzt)
- Git LFS gilt für neue große Dateien seit 0.19.0; ältere Commits bleiben unverändert (kein Umschreiben der Historie)
- Vorgemerkt: Sprint als `FSavedMove`-Flag; Physics Handle für schwere Objekte; Spielerinventar nur an den Besitzer replizieren; Reparatur weiterer Geräte (Motor, Haushaltsgeräte) mit eigenen Ersatzteilen; schwere Auktionsposten (Motoren) jetzt mit Sackkarre denkbar; Einrasten an Wänden im Baumodus; Stromausfall/Sicherung statt nur Warten; Förderbänder: Verteiler/Weiche, Bänder an Wänden entlang, Nachbarsuche cachen; Mitarbeiter: weitere Rollen (Reparaturtechniker, Produktionsarbeiter, Werkstattleiter), Mitarbeiter fahren den Gabelstapler, kleiner/großer LKW; Expansion: mehrere Standorte, Wagen sichtbar zum Hallentor, mehrere Standorte; Grafik: eigene Maschinenmodelle statt lackierter Blöcke, Hängelampen in den Hallen, UI-Klickgeräusche und Aufprallgeräusche für Gegenstände (Kenney-Sounds sind schon importiert)

## Bekannte Fehler

Keine.

## Nächste Schritte (nach Freigabe)

Veröffentlichung auf Steam: App ID, Auto-Cloud und Shopseite durch Mike (Anleitung `Docs/RELEASE_STEAM.md`), danach Shipping-Build mit der echten ID packen und hochladen.

## Testergebnisse

Siehe `TEST_REPORT.md`. Letzter Lauf 2026-10-20 (0.20.0): 130/130 Automation-Tests, Regression grün, gepacktes Spiel (Development und Shipping) geprüft. Davor 2026-10-19 (0.19.0): 129/129 Automation-Tests, Regression grün, gepacktes Spiel geprüft. Davor 2026-10-18 (0.18.1): 123/123 Automation-Tests, Regression grün, gepacktes Spiel geprüft. Davor (Phase 17.5): 116/116 Automation-Tests, Screenshot-Läufe P175/P175W/P175V im Editor und im gepackten Spiel, UI/Eingabe in 4 Auflösungen, Regression Netzwerktests A–X, I/J, 4 Spieler, Spielläufe P5–P16.
