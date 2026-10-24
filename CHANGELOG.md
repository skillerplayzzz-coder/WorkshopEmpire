# Changelog

## [0.23.0] – 2026-10-25 – Massive Quality Overhaul

Bericht mit allen Befunden und Tests: `Docs/OVERHAUL_REPORT.md`, Vorher/Nachher: `Docs/Overhaul_Vorher_Nachher.jpg`.

### Behoben
- **Absturz** beim Kauf eines einzelnen Teils, das per Paketdienst kommt (z. B. das Multimeter im Tutorial).
- Mitarbeiter blieben dauerhaft auf „kein Weg frei (Tor zu?)“ hängen; Bediener stehen jetzt an der Bedienseite der Maschine statt mit ausgestreckten Armen davor.
- Angenommene Aufträge lassen sich **abgeben** (mit Rückfrage) statt bis zum Ablauf zu blockieren.
- Deko (Besen, Kanister, Steckdosen …) blockiert keine Maschinenplätze mehr.
- Sicherungskasten, PC-Lampe und Möbel steckten in den Wänden; Seitenschneider fiel von der Werkbank.
- Untersuchen-Fenster flackerte bei laufenden Maschinen (Zeilen übereinander).
- Ampeln zeigten alle drei Lichter, zwei Hydranten nebeneinander, Firmenname auf dem Schild kaum lesbar, Hof nach dem Rausgehen kurz weiß überstrahlt.

### Neu / ersetzt
- Modelle statt Platzhaltern: Werkbänke, Deckenleuchten, Lieferpalette, alle Maschinen, Förderbänder und Gitterboxen, Schrotthaufen, Erdhügel, Ampeln, Kaffeebecher.
- Garage und Hallen eingerichtet: Elektroinstallation, Uhr, Dartscheibe, Schlauchtrommel, Schubladenschrank, Bodenmarkierungen, Stromkästen, Feuerlöscher, Paletten, Regale.
- Geräusche: Aufnehmen, Aufprall nach Material und Gewicht, Einstecken, Garagentor.
- Testlauf „Test 1“: neues Spiel mit Tutorial bis Fortsetzen, automatisch.

## [0.22.0] – 2026-10-24 – Quality Update: PC-System, Kundenaufträge, Autos

### Neu
- **Kundengeräte immer in der Hand:** [E] „PC aufnehmen“ – auch kleine Kundengeräte (Radio, Konsole, Akkuschrauber) landen nicht mehr im Inventar. Beim Tragen zeigt eine **grüne/rote Vorschau**, wo das Gerät auf der Werkbank liegen wird; eine belegte Werkbank nennt den Grund.
- **„In die Hand nehmen“** im Inventar-Kontextmenü und mit [R] am Boden: alles kann zur Werkbank getragen werden, nichts bleibt im Inventar hängen.
- **Kunden-PC öffnen:** defekte Teile im Arbeitsmodus tauschen (Seitenteil ab, Grafikkarte raus, neue rein, Funktionstest, abliefern). Das Gehäuse bleibt ganz.
- **RAM-Riegel einzeln** und **Wärmeleitpaste beim Einbau des CPU-Kühlers** (eigener Schritt mit der Spritze).
- **PC von Grund auf bauen:** leeres PC-Gehäuse und die Grundteile (CPU-Kühler, Prozessor, Festplatte, Mainboard, Netzteil) beim Ersatzteil-Express.
- **Neue Autos in der Stadt:** US-Limousine, Kombi, Pickup, alter Werkstatt-Pickup, SUV und Kleinwagen in echter Größe, mit Innenraum, Felgen, Lichtern und leeren Kennzeichen, in vielen Farben (statt der Spielzeugautos).

### Geändert
- Prompts: „E | PC aufnehmen“, „E | PC auf der Werkbank abstellen“, „E | PC bearbeiten“.
- Arbeitsmodus: Die Kamera geht nicht mehr ins Gehäuse oder durch Regale; nur das Teil unter dem Mauszeiger bekommt ein Schild, gesperrte Teile sagen, was zuerst raus muss.
- Große Geräte werden mittig und tiefer getragen.
- Heile Teile eines Kunden-PCs kommen beim Öffnen funktionsfähig heraus (nur das defekte Teil ist kaputt).
- Reifen, Ringe und andere runde Teile haben wieder ihr eigenes Material (vorher teils in der Gehäusefarbe).
- Spielstand Version 20 (zwei RAM-Riegel); ältere Spielstände laden weiter.
## [0.21.0] – 2026-10-21 – Phase 21: Realismus, Spieltiefe, Animationen und Spielgefühl

### Neu
- **Reparaturansicht:** Mausrad zoomt stufenlos zum Zeiger, mittlere Maustaste verschiebt, Doppelklick oder [F] holt die Kamera ganz nah an eine Schraube; alles gleitet weich. Nur was gerade geht, ist deutlich markiert.
- **Werkzeuge in der Hand:** Schraubendreher, Akkuschrauber, Zange, Messspitze, Lötkolben, Hammer usw. liegen auf der Werkbank, werden aufgenommen und arbeiten sichtbar an der Schraube. Gelöste Schrauben hängen am Magneten und landen in der **Schraubenschale**; beim Zusammenbau kommen sie von dort zurück.
- **Ersatzteile aus der Verpackung:** Karton öffnen, neue Teile verlieren ihre Schutzfolie, dann einsetzen und einrasten lassen.
- **Sichtbarer Zustand:** Geräte sind je nach Zustand verschmutzt, verkratzt, vergilbt oder beschädigt; nicht jeder Defekt ist sichtbar.
- **Diagnose:** einschalten und beobachten (Fehlerbild), dann Bauteile einzeln messen: „defekt“ oder „in Ordnung“.
- **Wartung:** Staub mit Druckluftspray ausblasen, alte Wärmeleitpaste abwischen und neue auftragen (PC, Konsole). Beim bestandenen Funktionstest geht das Licht an, der Bildschirm zeigt ein Bild, das Radio spielt.
- **Mitarbeiter:** greifen sichtbar ins Regal und in die Maschine, tragen die echten Teile, bedienen die Maschine. Kaffeepausen an der Kaffeemaschine, Plausch mit Kollegen, Sitzen auf dem Hocker.
- **Werkzeugwand:** Lochwand mit Haken; jedes Werkzeug hängt sichtbar an seinem Platz, leere Haken zeigen den Umriss. [R] alles aufhängen, [Q] das angeschaute Werkzeug nehmen. Werkzeug an einer Wand in der Nähe ist an der Werkbank griffbereit.
- **Paketbote:** Bestellungen bei den Versandhändlern und aus den Kleinanzeigen kommen als Pakete. Der Bote bringt sie höchstens alle paar Minuten zum Lieferbereich; Pakete aufheben und mit [Q] öffnen, drin ist genau das Bestellte.
- **Schwierigkeit beim neuen Spiel:** Entspannt, Realistisch, Experte oder jede Option einzeln: Diagnose erforderlich, Reparaturfehler (Schraubendreher rutscht ab, Teil rastet nicht ein), Kundenbewertungen, Wirtschaft (Preise), Reparaturkomplexität (Dauer, Anzahl der Fehler).
- **Kundenbewertungen und Firmenruf** (abschaltbar): Kunden schreiben zu ihren Sternen einen Satz; ein guter Ruf bringt mehr und besser bezahlte Aufträge, ein schlechter weniger.

### Geändert
- Neue Werkzeuge im Handel: Druckluftspray (Werkzeugladen) und Wärmeleitpaste (Ersatzteilhändler).
- Mitarbeiter finden Ziele auf Möbeln (z. B. die Kaffeemaschine auf dem Tisch) jetzt zuverlässig und werden beim Festhängen nicht mehr in den Boden gesetzt.
- Spielstand Version 19 (Regeln, Pakete, Bewertungen). Ältere Spielstände laden weiter und spielen sich wie bisher (ohne Paketbote und Reparaturfehler).
## [0.20.0] – 2026-10-20 – Phase 20: Werkstatt-Ausbau und Release-Vorbereitung

### Neu
- **Firmenlogo:** Im Ferrit OS unter Gestaltung → Firmenlogo wählst du Symbol (12), Symbolfarbe, Hintergrund, Farbe der Arbeitskleidung und den Firmennamen. Das Logo erscheint über dem Tor, am Firmenschild, auf beiden Seiten des Transporters und auf dem Overall; jede Änderung gilt sofort für alle Mitspieler und wird gespeichert.
- **Mitarbeiter als echte Figuren:** gleiche Körper, Gesichter und Kleidung wie Spieler und Kunden, im Overall in deiner Firmenfarbe mit Logo.
- **Neue Reparaturen:** Fernseher, Kofferradio, Spielkonsole und Akkuschrauber mit eigenen Fehlern und Ersatzteilen (Lautsprecher, Laufwerk, Akku, Kohlebürsten u. a.), zu kaufen beim Ersatzteilhändler; Laufkunden und Aufträge bringen die Geräte.
- **Werkstattradio** auf der Werkbank mit 6 Funk-Titeln: [E] an/aus, [Q] nächster Titel, alle Mitspieler hören dasselbe.
- **Kaffeemaschine:** für 0,50 € drei Minuten 15 % schneller laufen und 20 % schneller arbeiten, Restzeit an der Uhr.
- **Werkstattkatze „Schraube“:** streift durch die Werkstatt, setzt sich, schläft schnurrend, miaut; [E] streicheln.
- **Wetter:** klar, bewölkt, Regen und Gewitter mit Blitz und Donner; graue Wolkendecke, weiche Schatten, unter dem Dach trocken und gedämpft. Das Wetter steht an der HUD-Uhr.
- **Einstellungen:** Schieberegler für Bewegungsunschärfe (neu), Sichtfeld, Mausempfindlichkeit, UI-Größe und Lautstärke; alle wichtigen Tasten frei belegbar (belegte Tasten werden getauscht, „Tasten zurücksetzen“).
- **Release-Vorbereitung:** Shipping-Build, ein Skript trägt die eigene Steam App ID überall ein, Steam-Cloud-Pfade für die Spielstände, Upload-Vorlagen und ein 52-Sekunden-Trailer aus echten Spielszenen (Anleitung `Docs/RELEASE_STEAM.md`).

### Geändert
- Kupferkabel gibt es jetzt auch beim Ersatzteilhändler.
- Speicherstand Version 18 (Firmenmarke); ältere Spielstände laden weiter.

## [0.19.1] – 2026-10-19 – Nachbesserungen nach Mikes Spielvideo

### Geändert
- **Normale Körper statt Superhelden:** Arme, Schultern, Brust und Beine der Figuren sind schmaler (Kopf, Hände und Füße unverändert); alle Kleidungsstücke sind neu aus diesen Körpern geschnitten, Hoodie und Jacke etwas weniger weit.
- **Echter Spiegel:** Das Spiegelbild hängt jetzt davon ab, wo du stehst und hinschaust, wie bei einem echten Spiegel.
- **Bewegungsunschärfe** fast ausgeschaltet (das Bild verwischte bei jeder Mausbewegung).
- **Zerlegen von Monitor, Radio & Co.:** Die Rückwand hat das Material des Geräts statt einer grauen Platte, Platinen und Bauteile sind kleiner und liegen im Gerät statt herauszuragen.
## [0.19.0] – 2026-10-19 – Phase 19: Aufträge, Kleinanzeigen, Tag und Nacht, Kleidung, Spiegel

### Neu
- **PC-Aufträge**: Kunden wollen einen funktionierenden PC mit Mindest-Benchmark oder lassen ihren PC aufrüsten. Neue Teile (Grafikkarte Pro, 8-GB-Speicher, schneller Prozessor, schnelle SSD) passen in dieselben Plätze; nach dem Funktionstest zeigt der PC seine Punkte. Abgelieferte Aufträge bekommen 1–5 Sterne, 4★ +10 %, 5★ +25 %.
- **Kleinanzeigen** im Ferrit OS: gebrauchte Sachen von Privatleuten kaufen, handeln (Angebot, Gegenangebot), eigene Sachen inserieren und an Käufer verkaufen.
- **Tag und Nacht**: Sonne zieht über den Himmel, abends Mondlicht und Straßenlaternen, Öffnungszeiten 8–19 Uhr, Uhr im HUD (Sonne/Mond) und im Ferrit OS.
- **Laufkundschaft** am Tor: bringt Geräte zur Reparatur, will etwas verkaufen oder ein Teil kaufen; [E] annehmen, [Q] wegschicken. Alle Spieler bekommen eine Meldung, wenn jemand wartet.
- **Echte Kleidung**: Overall, Polo mit Chino, Hoodie mit Kapuze und Jeans, T-Shirt mit Arbeitshose und Knieschonern, Jacke mit Stehkragen und Hose; Sneaker, Arbeitsschuhe, Stiefel – für Mann und Frau.
- **Mehr Gesichter**: Schnurrbart, Kinnbart, Dreitagebart, Sommersprossen, Make-up (8 Männer-, 5 Frauengesichter).
- **Spiegel** in der Werkstatt: du siehst dich selbst und änderst dort mit [E] Kleidung, Gesicht und Frisur – die Mitspieler sehen es sofort.
- **Komfort**: Suchen und Sortieren in jedem Lager, Verkaufen direkt aus Werkbank- und Regallagern, Upgrade „Werkzeugtasche“ (+5 Plätze, Werkzeuge wiegen nichts).

### Geändert
- Gabelstapler mit eigenem Modell (Gegengewicht, Sitz, Lenkrad, Schutzdach, Hubmast, Gabeln) und neuem Stellplatz.
- Oberfläche: weiche Schatten und runde Ecken bei Inventar, Menüs, Pause und Spiegel; Zahlen deutsch geschrieben („1.550“, „2,5 kg“).

### Behoben
- Ferrit OS zeigte die Uhrzeit des echten Computers.
- Gabelstapler stand im Werkzeugkasten und im Verteilerkasten.
- Lange Namen in Inventarfeldern wurden an beiden Rändern abgeschnitten (jetzt „…“).
- Beschriftungen in schmalen Einstellungszeilen liefen unter die Pfeile.

### Technisch
- Speicherstand Version 17 (Teile-Varianten, Bewertungen, Kleinanzeigen, Uhrzeit); 129 Automation-Tests; neue Bilderläufe `-WEShots=P19C` und `P19G`.
- Git LFS für große Dateien, lokales Backup-Repository.
## [0.18.1] – 2026-10-18 – Nachbesserungen nach Phase 18

### Neu
- PC wieder zusammenbauen: Ansicht „Zusammenbauen“ in der Arbeitsansicht, Teile aus Inventar oder Werkbanklager einsetzen und festschrauben, Reihenfolge wie beim echten PC
- Funktionstest nach dem Zusammenbau: mit guten Teilen läuft der PC, defekte Teile werden genannt; ungetestete PCs verkaufen sich wie defekte
- Transporter: Ladung wird einzeln von den Händen in den Laderaum getragen und einzeln wieder ausgeladen

### Geändert
- Alle selbst erzeugten Sounds durch freie Aufnahmen ersetzt (Pixabay, Kenney)
- Geparkte Autos aus dem Kenney Car Kit (CC0, ohne Marken)
- Werkbanklager 24 Plätze / 100 kg (vorher 12 / 60), große Werkbank 36 Plätze
- CD-Laufwerk und Gehäuselüfter sind optional: ein PC ohne sie gilt als vollständig (etwas weniger wert)

### Technisch
- Speicherstand Version 16 (eingebaute Teile); 123 Automation-Tests; neuer Bilderlauf `-WEShots=P18R`

## [0.18.0] – 2026-10-18 – Phase 18: PC-System, Transporter, Charaktereditor, UI und Stadt

### Neu
- **PC zerlegen wie in echt**: Der alte PC liegt auf der Seite auf der Werkbank, das Seitenteil ist verschraubt. Darin sitzen Mainboard, Grafikkarte, zwei Speicherriegel, CPU-Kühler, Prozessor, Festplatte, SSD, Gehäuselüfter, Netzteil und CD-Laufwerk an ihren Plätzen. Teil anklicken, Stecker ziehen, Schrauben lösen, Clips und Hebel öffnen, dann wird das Teil herausgezogen. Die Reihenfolge ist frei, solange nichts im Weg ist (der Prozessor erst nach dem Kühler, das Mainboard erst wenn alles darauf ausgebaut ist). Kamera mit rechter Maustaste drehen, Mausrad zoomt, das gewählte Teil wird hervorgehoben. Jede Schraube und jedes Teil landet als Gegenstand im Werkbanklager.
- **Neue Gegenstände**: CPU-Kühler, Prozessor, Festplatte, SSD, Gehäuselüfter; Grafikkarte, Arbeitsspeicher, Netzteil und Mainboard haben neue Modelle.
- **Neuer Transporter**: amerikanischer Kastenwagen mit langer Haube, Grill, Scheinwerfern, Stahlfelgen, großen Spiegeln, Türgriffen, Innenraum und Laderaum mit Regal. Die Hecktüren öffnen sich, wenn man hinter dem Wagen steht oder etwas ein- und auslädt, die Fahrertür, wenn der Fahrer einsteigt. Die Ladung steht sichtbar im Laderaum.
- **Charaktereditor** beim neuen Spiel und im Multiplayer-Menü („Aussehen ändern“): Mann oder Frau, Gesicht, Hautton, Frisur, Haarfarbe, Kleidung (Arbeitsoverall, Polo und Chino, Hoodie und Jeans, T-Shirt und Arbeitshose, Jacke und Hose) in fünf Farben, Schuhe, Cap oder Mütze, Sonnenbrille. Die Figur steht drehbar im Menü. Mitspieler sehen sie im Spiel, mit [V] siehst du sie selbst von hinten.
- **Baumenü** mit Kategorien (Werkbänke, Lagerung, Maschinen, Werkstattausstattung, Dekoration, [T] wechselt), Bild, Preis und Kurzbeschreibung je Möbel; die Vorschau zeigt das Möbel durchsichtig in Grün (passt) oder Rot (passt nicht).
- **Stadt statt Weltende**: zwei Kreuzungen mit Ampeln und Zebrastreifen, Tankstelle, Parkplatz, Autowerkstatt, Imbiss, Brachfläche mit Plakatwand, geparkte Autos, Sträucher, Unkraut und Bäume. Die Straßen laufen weiter bis zum Horizont; am Rand des Spielbereichs sperren Baustellen die Fahrbahn, dazwischen stehen Blechzäune. Dahinter Gebäude und eine Skyline.
- Tutorial-Karte kleiner und dezenter.

### Geändert
- Teile-Werte: Grafikkarte 22 €, Arbeitsspeicher 6 €, Mainboard (ohne CPU und Kühler) 9 €. Ein ganz zerlegter PC bringt etwas mehr Teile, dauert aber auch länger.
- Speicherstand Version 15 (ausgebaute Teile, Aussehen); ältere Spielstände werden übernommen.

### Behoben
- Mitarbeiter fanden den Weg aus der Garage nicht (der Fahrer blieb vor dem offenen Tor stehen).

## [0.17.5] – 2026-10-18 – Phase 17.5: Visual Overhaul, Gameplay Polish & Immersion

### Neu
- **Neue Oberfläche** in einem Werkstatt-Stil (Stahl, Signalgelb, kantige Formen, eigene Icons, Klickgeräusche, kurze Einblendungen): HUD mit Auftragskarte, Statuskarte, Tastenhinweisen und Meldungen mit Symbol; Inventar mit Bildern, Zustandsstreifen, Detailkarte und Rechtsklick-Menü; Pausemenü.
- **Item-Bilder**: jeder Gegenstand hat ein Bild, gerendert aus seinem Modell.
- **Einstellungen**: Grafikqualität, Auflösung, Fenstermodus, VSync, Bildrate, UI-Skalierung, Mausempfindlichkeit, Maus invertieren, Sichtfeld, Lautstärke – im Hauptmenü und im Spiel.
- **Hauptmenü** neu: Spiel fortsetzen (neuester Spielstand), Neues Spiel (Firmenname, Tutorial ja/nein), Multiplayer, Einstellungen, Beenden; dahinter die Werkstatt im Industriegebiet.
- **Werkstatt-PC** statt Marktterminal: Schreibtisch mit Monitor, Tastatur, Maus, Lampe und Stuhl. [E] setzt sich davor, die Ansicht fährt auf den Bildschirm, und „Ferrit OS“ startet: Desktop, Taskleiste, Fenster zum Verschieben und Minimieren, Programme Marktplatz, Aufträge, Auktionen, Personal & Logistik, Finanzen, Unternehmen, Gestaltung, Hilfe. Andere Spieler sehen auf dem Monitor, wer gerade daran sitzt.
- **Arbeitsansicht an der Werkbank**: [E] (Zerlegen) oder [Q] (Reparieren) auf ein Gerät zeigt es von oben. Werkzeug wählen (1–8, Mausrad), dann auf jeder Schraube, Klammer, jedem Stecker, der Abdeckung und den Teilen die Maustaste halten – in der richtigen Reihenfolge. Reparieren: Messpunkte prüfen, defektes Teil ausbauen, Ersatzteil einsetzen, einschalten, reinigen. Schrauben drehen sich heraus, Teile wandern in die Ablage, jedes Werkzeug klingt anders. Was ein Schritt verlangt, ergibt sich aus den Gerätedaten, deshalb ist jedes Gerät anders. „Automatisch [Q]“ macht den Schritt wie bisher; Mitarbeiter arbeiten weiter so.
- **Industriegebiet**: Straße mit Gehwegen, Gullydeckeln und Hydrant, Holzmasten mit Leitungen (eine führt zur Garage), Straßenlampen, Maschendrahtzaun mit Tor zur Straße, Nachbarhäuser aus Backstein und Beton, Wellblechhalle, Containerlager, Müllcontainer, abgedecktes Auto, Paletten, Fässer, Absperrungen, Firmenschild. Mit den Ausbaustufen kommen Palettenstapel, Container, ein zweites Schild und mehr dazu.
- **Werkstatt gestalten** (Programm „Gestaltung“): pro Gebäude Wandfarbe (11), Boden (8), Licht (5) und Tor (7); manche erst ab einer höheren Stufe. Gespeichert und für alle Mitspieler gleich.
- **13 Deko-Möbel** im Baumodus (Pflanzkübel, Kreidetafel, Fernseher, Stuhl, Fassofen, Warnschild, Feuerlöscher, Kompressor, Standbohrmaschine, Getränkekisten, Industrielampe, Kaffeewagen, abgedecktes Auto).
- **Verkaufsschutz**: Werkzeuge und Tragehilfen sind persönliche Ausrüstung und werden ohne Freigabe nie verkauft; Gegenstände lassen sich als Favorit markieren oder sperren (Rechtsklick im Inventar); seltene und wertvolle Stücke fragen vor dem Verkauf nach. Händler, Aufträge und Verkaufstouren halten sich daran. Der Marktplatz trennt Verkaufsware und geschützte Teile.
- **Alte Gebäude verkaufen** (Programm „Unternehmen“): 35 % des Kaufpreises zurück, mit Rückfrage; nur wenn ein größeres Gebäude da ist und nichts mehr darin steht – „Räumen“ bringt lose Gegenstände zur Lieferpalette und Mitarbeiter hinaus. Strom und Personalplätze sinken entsprechend.
- **Tutorial** (beim neuen Spiel angeboten, jederzeit überspringbar, im Pausemenü neu startbar): 11 kurze Schritte von Laufen über Inventar, Zerlegen, Werkstatt-PC, Verkaufen, Auftrag und Reparieren bis Speichern, mit Markierungen in der Welt; der Fortschritt wird gespeichert.
- **Transporter**: alter, verbrauchter Transporter mit Dachträger und Rost, moderner Lieferwagen mit Zierstreifen; Räder drehen sich beim Fahren; der Wagen parkt mit der Nase zur Straße und fährt durchs Tor.

### Behoben
- Absturz im gepackten Spiel beim Anzeigen der nächsten Firmenstufe (seit 0.12.0, im Editor nicht sichtbar).
- Rolltor war von innen durchsichtig.
- F5 schaltete im Entwicklungs-Build zusätzlich eine Shader-Ansicht ein.

### Technisch
- Spielstand-Version 14 (Gestaltung, Tutorial, Markierungen an Gegenständen); ältere Spielstände laden weiter.
- Gepacktes Spiel: beide Karten und alle per Pfad geladenen Inhalte werden mitgekocht.
- Neue Inhalte aus CC0-Quellen und eigene Modelle, siehe `Docs/ASSET_SOURCES.md`; Nachbarschaft: `Tools/Maps/world_neighbourhood.py`.
- Dev-Läufe `-WEShots=P175` (Menüs und Oberfläche), `P175W` (Arbeitsansicht), `P175V` (Transporter und Welt); Konsolenvariable `we.WorkView`.
- 6 neue Tests, insgesamt 116.

## [0.17.0] – 2026-10-18 – Phase 17: Qualitätssicherung

### Behoben
- Gegenstände konnten durch den Hofboden fallen und verschwanden für Mitspieler. Jetzt mit genauer Kollision, Bodenplatte unter dem Hof, und was trotzdem fällt, landet auf der Lieferpalette.
- Baumodus-Liste: lange Einträge laufen nicht mehr über den Rand.
- Marktplatz: lange Wörter laufen nicht mehr in den Preis, Bewerberbeschreibungen nicht mehr in den Lohn, der Titel klebt nicht mehr am ersten Reiter.
- Spiel-Build schlug bei einzeln kompilierten Dateien fehl (fehlende Includes).

### Leistung
- Ruhende Gegenstände werden nicht mehr übertragen, bis sich etwas an ihnen ändert.
- Gemessen: volle Fabrik (28 Maschinen, 20 Mitarbeiter, 400 Gegenstände) 54–68 fps in 1080p; Speichern 11 ms, Laden 134 ms; 30 min ohne Speicheranstieg.

### Qualitätssicherung
- Multiplayer-Stress mit 4 Clients, schlechter Verbindung und 30 min Dauerlauf: 14.400 Aktionen, 0 Fehler, Konto überall gleich.
- UI- und Eingabetests bei 1024×768 bis 2560×1440.
- 5 neue Tests (beschädigte Spielstände, wiederholtes Speichern/Laden, alte Versionen, große Welt, fallende Gegenstände), insgesamt 110.

### Technisch
- Dev-Läufe `-WEShots=P17` (Leistung, `-WEStressMinutes=N` Dauerlauf), `P17UI_*` (Tasten und Fenstergrößen), `-WESmokeRole=Z` (Zufallsaktionen), Server `-WEServerMonitor` und `-WESpawnItems=N`; Konsolenvariable `we.ItemDormancy`. Doku: `Docs/QA.md`.

## [0.16.0] – 2026-10-17 – Phase 16: Balancing und Langzeitmotivation

### Neu
- Größere Abholtouren für größere Firmen: Elektroschrott-Container (ab Große Werkstatt), Kabel- und Motorenschrott und Großauflösung Bürogebäude (ab Gewerbehalle), Fabrikauflösung (ab Fabrik). Gesperrte Touren zeigen "Benötigt Stufe …". Manchmal ist ein Sammlerstück dabei.
- Lkw mit Ladekran (12.000 €, nach der Gewerbehalle): 40 Plätze, 3 t, Touren 2,4-mal so schnell.
- Edelmetall-Rückgewinnung (9.000 €, Fabrikhalle): macht aus Elektronikschrott Edelmetallkonzentrat, aus Keramik-Prozessoren ebenfalls.
- 14 neue Aufträge: aufbereitete PCs, Netzteile, Elektromotoren, Sammleraufträge für jedes Sammlerstück, Schulausstattung, Kupferdrahtspulen, Aluminiumballen und -barren, Edelmetallkonzentrat, Kabelpaletten, Großprojekte ab Stufe 5.
- Meilensteine: 31 Ziele für die ganze Firma (Zerlegen, Reparieren, Aufträge, Maschinen, Touren, Umsatz, Stufen, Personal, Sammlerstücke, Firmenwert bis 1 Mio. €) mit Prämie aufs Firmenkonto und Meldung an alle; Fortschritt im Reiter "Unternehmen".

### Balancing
- Langzeit-Playtest als Simulation über 60 Stunden auf den echten Spieldaten. Vorher blieb das Einkommen ab Stunde 6 bei ~900 €/h stehen; jetzt (allein) Große Werkstatt nach ~10 h, Gewerbehalle ~18 h, Fabrik ~32 h, Industrieunternehmen ~45 h. Zu zweit schneller.
- Sortiermaschine gewinnt mehr Kupfer und Aluminium aus Elektronik.
- Transporter 2.000 € (vorher 2.400 €), Werkstatt-Erweiterung 5.000 € (vorher 6.000 €), Fabrikhalle 75.000 € (vorher 90.000 €).

### Behoben
- Der Firmenwagen stand seit 0.15.0 in der Garage statt vor dem Tor.
- Stufen-Meilensteine kommen sofort beim Kauf.
- Fahrzeug zeigt "3000 kg" statt "3,000 kg".

### Technisch
- `UWEMilestoneComponent` am GameState (Statistik, Meilensteine, repliziert), Katalog `DA_MilestoneCatalog`; Speicherversion 13.
- Touren mit `MinTier`, Tourposten mit `Chance`; dritte Fahrzeugstufe (`TruckStats`).
- Test `WorkshopEmpire.Balance.ProgressionCurve` (Simulation, Bericht `Saved/Balance/ProgressionCurve.md`, schlägt fehl, wenn die Kurve die Zielfenster verlässt) und 2 weitere neue Tests (insgesamt 106); Screenshot-Lauf P16. Doku: `Docs/BALANCING.md`.

## [0.15.0] – 2026-10-17 – Phase 15: Grafik und Atmosphäre

### Neu
- Neue Beleuchtung: globale Beleuchtung und Reflexionen (Lumen), weiche Schatten, Sonne, Wolkenhimmel und leichter Dunst mit Lichtstrahlen durchs Tor; Leuchtstoffröhren in Werkstatt und Hallen; das Auge passt sich zwischen hellem Hof und Werkstatt an.
- Echte Materialien: Ziegelgarage, verputzte Innenwände, fleckiger Betonboden, Asphalthof, Wellblech- und Industriehallen, Blechdächer, lackiertes und rostiges Metall, Riffelblech, Sperrholz, OSB. Wände werden zum Boden hin schmutziger.
- 51 Modelle: Werkzeuge, Röhrenmonitor, Sackkarre, Werkzeugkisten, Stahlregale, Kunststoffkisten, Werkstattwagen, Rolltor, Leitern, Feuerlöscher, Kompressor, Schweißwagen, Generator, Fässer, Reifen, Kanister, Zementsäcke, Lampen.
- Rolltor, das sich beim Öffnen aufrollt.
- Spieler und Mitarbeiter als animierte Figuren (Spielerfarbe bzw. Rollenfarbe) mit Schrittgeräuschen.
- Partikeleffekte: Staub und Späne an Werkbank und Schredder, Dampf und Funken an der Presse, Rauch und Glut am Schmelzofen, Schwebstaub in der Werkstatt. Laufende Maschinen vibrieren.
- Sounddesign: Maschinen mit Anlauf, Dauerton, Auslauf und Geräusch je Vorgang; Raumton in der Werkstatt, draußen Wind und Vögel; der Firmenwagen fährt sichtbar los und kommt zurück. Geräusche werden mit Entfernung leiser.
- Grafikqualität im Hauptmenü und im Menü [Esc]: Niedrig, Mittel, Hoch (Standard), Episch.
- Hauptmenü-Kulisse mit echten Modellen.

### Leistung
- RTX 3060, 1080p: Hoch 71–83 fps, Episch ca. 30 fps, Mittel 113–148 fps (gemessen im Editor-Spielmodus).

### Technisch
- `AWEFx`/`AWEFxEmitter` (CPU-Partikel als instanziertes Mesh, keine Niagara-Assets), `WESurface` + Master-Material `M_WESurface` (weltprojiziert), `WEMannequin`, `UWEFootstepComponent`, `WEGraphics`; Maschinen-Rückmeldung als Daten in `UWEMachineDefinition`; Möbel mit Modell (`VisualMesh`, `bTileVisual`, Deko).
- Asset-Pipeline: `Tools/Assets/download_assets.py` (CC0 von Poly Haven und Kenney, nach `SourceArt/`, nicht im Git) und `import_assets.py`; Lizenzen in `Docs/ASSET_SOURCES.md`. UE-Mannequins aus den Engine-Vorlagen.
- Rendering-Einstellungen in `DefaultEngine.ini` (Lumen, Virtual Shadow Maps, Distance Fields, Belichtung in EV100), Standardqualität in `DefaultGameUserSettings.ini`.
- 5 neue Automation-Tests (insgesamt 103); Screenshot-Lauf P15 mit FPS-Messung je Ansicht.

## [0.14.0] – 2026-10-17 – Phase 13: Multiplayer-Integration und Phase 14: Steam-Integration

### Neu
- Hauptmenü beim Start: neues Spiel hosten, Spielstand laden und hosten (Liste mit Datum und Firmenkonto), Spiel beitreten (gefundene Spiele mit Host, Spielern, Ping und Version oder per IP-Adresse), Name (ohne Steam), Beenden.
- Koop für 2–4 Spieler mit Drop-in: Mitspieler kommen und gehen jederzeit, alle sehen "… ist beigetreten" bzw. "… hat das Spiel verlassen".
- Menü im Spiel auf [Esc] (das Spiel läuft weiter): Spielerliste mit Host und Ping, Mitspieler entfernen (Host), Speichern (Host), Steam-Freunde einladen, Zum Hauptmenü, Spiel beenden.
- Wer geht, abstürzt oder die Verbindung verliert, legt Getragenes ab; Position und Inventar warten beim Host, kommen beim Wiederkommen zurück und stehen im Spielstand.
- Beendet der Host das Spiel, wird automatisch gespeichert und alle kehren mit Meldung ins Hauptmenü zurück.
- Klare Meldungen im Hauptmenü: Host hat beendet, vom Host entfernt, Spiel voll, andere Spielversion, Verbindung verloren, Zeitüberschreitung, Adresse nicht erreichbar, Einladung ungültig.
- Steam: Spiele laufen als Steam-Lobby über Steams Relay-Netz (keine Portfreigabe), Freunde einladen über das Steam-Overlay, Beitreten über die Steam-Freundesliste oder "Steam-Spiele suchen". Ohne Steam automatisch LAN/IP.

### Multiplayer und Sicherheit
- Der Server prüft beim Beitritt Spielerzahl und Spielversion; nur der Host speichert, lädt und entfernt Spieler. Mit 4 Spielern über das echte Hauptmenü getestet (inkl. Absturz eines Mitspielers).

### Technisch
- `UWESessionSubsystem` (Sessions, Suche, Beitritt, Einladungen, Fehlermeldungen), `AWEMenuGameMode`/`AWEMenuPlayerController`, `UWEMainMenuWidget`, `UWEPauseMenuWidget`; neue Karte `L_MainMenu` (Skript `build_menu_map.py`), jetzt Startkarte.
- Plugins OnlineSubsystem, OnlineSubsystemNull, OnlineSubsystemSteam, SteamSockets; Steam-NetDriver mit IP-Fallback; Steam-Test-App 480.
- Speicherformat Version 12 (Spielernamen, gegangene Spieler).
- 3 neue Automation-Tests (insgesamt 98); Netzwerktest-Rollen für 4 Spieler (MH, M1–M3, M5) und Steam (SH, SC); Screenshot-Läufe P13/P13C und P14. Alle automatischen Läufe mit `-nosteam`.

## [0.12.0] – 2026-10-17 – Phase 12: Unternehmensexpansion

### Neu
- Drei Gebäude zum Kaufen ("Werkstatt verbessern"), die sofort auf dem Gelände stehen, mit eigenem Tor, Licht und Baufläche: Werkstatt-Erweiterung (6.000 €), Gewerbehalle (25.000 €), Fabrikhalle (90.000 €). Sie bringen mehr Stromanschluss (+5,5 / +22 / +44 kW) und Platz für mehr Mitarbeiter (+2 / +4 / +8).
- Unternehmensstufen: Kleine Garage → Große Werkstatt → Gewerbehalle → Fabrik → Industrieunternehmen (Fabrik und 250.000 € Firmenwert). Eine neue Stufe wird allen Spielern gemeldet.
- Neue Produktion: Schmelzofen (Aluminium- und Kupferbarren), Verpackungsanlage (Paletten Stahlwinkel und Kabel); Hochregal und Großes Materiallager. Diese Einrichtung ist erst ab der passenden Stufe kaufbar ("Benötigt: …" im Baumodus).
- Industrieaufträge mit eigener Liste (bis zu 2 Angebote, 2 gleichzeitig): größere Mengen, bessere Bezahlung, Vertragsstrafe bei verpasster Frist. Lieferverträge mit 3–4 Lieferungen, jede sofort bezahlt.
- Neue Wagentour "Auslieferung": liefert alle angenommenen Aufträge, deren Ware im Laderaum liegt.
- Neuer Reiter "Unternehmen" am Marktplatz: Stufe, nächste Stufe, Firmenwert, Strom, Personal, Einnahmen und Ausgaben je Bereich (letzte Stunde und insgesamt) und die laufenden Aufträge mit Lieferungen und Strafe.

### Multiplayer und Sicherheit
- Gebäude, Stufe, Firmenwert und Buchhaltung kommen aus dem replizierten Serverzustand; gesperrte Einrichtung wird auch vom Server abgelehnt. Im Netzwerk mit 2 Spielern getestet.

### Technisch
- Gebäude sind Upgrades; Actors mit Tag `WEUpgrade.<Id>` werden auf jedem Rechner ein- und ausgeblendet (inkl. Kollision und Navigationsnetz).
- `EWEBookingCategory`-Buchhaltung im `UWECompanyComponent`; Auftragsvorlagen mit Mindeststufe, Industrie-Flag und Lieferungen.
- Speicherformat Version 11 (Lieferungen von Aufträgen, Buchhaltung).
- Map mit drei Gebäuden, 4 neue Gegenstände, 2 Maschinen, 4 Möbel, 6 Industrieauftragsvorlagen per Skript.
- 7 neue Automation-Tests (insgesamt 95) inkl. Balance-Messung; Netzwerktest-Rollen W/X; Screenshot-Lauf P12.

## [0.11.0] – 2026-10-16 – Phase 11: Mitarbeiter und Logistik

### Neu
- Mitarbeiter einstellen im neuen Reiter "Personal & Logistik": Lagerarbeiter, Maschinenbediener, Fahrer; 3 Bewerber (alle 10 Minuten neu), höchstens 6 Mitarbeiter, Erfahrung 1–3 (schneller, teurer).
- Aufgaben in der Werkstatt zuweisen: [E] auf den Mitarbeiter, dann Quelle/Ziel anschauen und [E]; [Q] Pause, [T] Material, [X] zweimal Entlassen, [F] Details.
- Lagertransporte zwischen Lagern, Maschinen und Wagen (nur was das Ziel verarbeitet); Maschinenbedienung (einschalten, warten mit Wartungsset, Produkte einlagern, Maschine 15 % schneller).
- Löhne alle 5 Minuten für die gearbeitete Zeit; ohne Geld legen alle die Arbeit nieder, bis die Schuld bezahlt ist.
- Firmenwagen "Alter Transporter" und "Moderner Lieferwagen" (Upgrades): Laderaum, Verkaufsfahrten (+15 %), Großeinkäufe Altgeräte und Industriemotoren; braucht einen Fahrer.
- Gabelstapler (Upgrade): hebt bis 1.500 kg; neuer Großer Industriemotor (180 kg); der Schredder nimmt jetzt bis 300 kg.
- Mitarbeiter laufen über ein Navigationsnetz, das sich der Einrichtung anpasst, und warten, wenn kein Weg frei ist.

### Multiplayer und Sicherheit
- Einstellen, Entlassen, Pause und Zuweisen über Server-RPCs (Terminal-Reichweite bzw. Nähe zum Mitarbeiter, Anfrage-ID beim Einstellen); Mitarbeiter, Wagen und Gabelstapler replizieren ihren Zustand. Im Netzwerk mit 2 Spielern getestet.

### Behoben
- Ein gefahrener Gabelstapler bekam nach einem Upgrade wieder Kollision.

### Technisch
- `AWEEmployee`, `UWEStaffComponent`, `UWEStaffCatalog`/`UWETourCatalog`, `AWEVehicle`, `AWEForklift`; neue Module `AIModule` und `NavigationSystem`; `RecastNavMesh` mit Laufzeit-Aufbau.
- Speicherformat Version 10 (Mitarbeiter mit Aufgaben, Bewerber, Löhne, Touren).
- Neues Skript `Tools/Maps/build_staff.py`; Map mit Wagen, Gabelstapler, Navigationsnetz und Personal-Startpunkt.
- 7 neue Automation-Tests (insgesamt 88) inkl. Balance-Messung von Touren und Lohn; Netzwerktest-Rollen U/V; Screenshot-Lauf P11.

## [0.10.0] – 2026-10-16 – Phase 10: Produktion und Automatisierung

### Neu
- Förderbänder (1 m 25 €, 2 m 45 €): im Baumodus aufstellen, nehmen am hinteren Ende Produkte aus Maschine oder Materiallager, geben vorn an Band, Maschine oder Lager ab; über Eck verbindbar, stauen sich bei vollem Ziel. [E] auflegen bzw. herunternehmen, [Q] anhalten, [F] zeigt Quelle und Ziel.
- Maschinenverbindungen und automatischer Materialfluss: eine Linie Zuführband → Schredder → Sortiermaschine → Materiallager → Fertigungsautomat arbeitet ohne Spieler.
- Materiallager (150 €, 30 Plätze, 400 kg): nimmt vom Band an, gibt aufs Band ab; [T] Ausgabe Alles / ein Material / Aus; schickt nur, was die Maschine am Ende der Linie annimmt.
- Produktionsrezepte wählbar mit der neuen Taste [T] (Automatisch oder fest) an jeder Maschine; Rezepte mit zweiter Zutat.
- Fertigungsautomat (600 €, 6 kW) mit Kupferdrahtspule, Verlängerungskabel und Stahlwinkel-Set (Produkte im Zustand "Neu"); die Gebrauchtbörse kauft Produkte; 4 neue Produktionsaufträge.

### Multiplayer und Sicherheit
- Bänder replizieren ihre Gegenstände als Daten (keine Actors) und laufen auf Clients flüssig weiter; alle Aktionen über Server-RPCs mit Reichweitenprüfung. Im Netzwerk mit 2 Spielern getestet.
- Versiegelte Retourenpaletten kauft auch nach der neuen Produktkategorie niemand ungeöffnet.

### Behoben
- Untersuchen-Fenster schnitt lange Angaben (Maschinen, Bänder) ab.

### Technisch
- `AWEConveyor`, `AWEProductionStorage`; Materialfluss im Simulationsschritt von `UWEPowerGridComponent` (kein Tick); `FindPortActor`, `FindLineDestination`.
- Neue Interaktion [T] (`GetSettingInteraction`/`InteractSetting`), 21 Tastenbelegungen.
- Speicherformat Version 9 (Bänder mit Gegenständen, Lagerausgabe, gewähltes Rezept).
- 6 neue Automation-Tests (insgesamt 81) inkl. Balance-Messung der Produkte; Netzwerktest-Rollen S/T; Screenshot-Lauf P10.

## [0.9.0] – 2026-10-15 – Phase 9: Erste Industriemaschinen

### Neu
- Drei Maschinen, aufgestellt über den Baumodus: Schredder (450 €, 7,5 kW), Sortiermaschine (380 €, 4 kW), Metallpresse (320 €, 5,5 kW).
- Erste Verarbeitungskette: Geräte und Schrott → Schreddergut → Kupfergranulat, Aluminiumschrott, Stahlschrott, Kunststoff, Elektronikschrott → Stahlballen, Aluminiumballen, Kupferpaket. Kabelkartons werden direkt zu Kupfergranulat.
- Maschinenbedienung: [E] Eingang öffnen bzw. getragenes Gerät hineingeben, [Q] ein/aus, [R] Produkte entnehmen, [X] warten, [F] Status. HUD zeigt Zustand, Fortschritt und Wartung der anvisierten Maschine.
- Maschinenzustände: Aus, Bereit, Läuft, Ausgabe voll, Stromanschluss überlastet, Wartung nötig. Ausschalten pausiert einen Vorgang.
- Strom: 11 kW Hausanschluss, Maschinen starten nur mit freier Leistung; Upgrade "Starkstromanschluss" (+11 kW, 300 €); Stromrechnung alle 5 Minuten vom Firmenkonto; Anzeige "Strom x / y kW".
- Verschleiß je Vorgang, langsamer unter 30 %, Stillstand bei 0 %; Wartungsset (Werkzeug-Versand) bringt die Maschine auf 100 %.
- 8 neue Gegenstände: Schreddergut Elektronik/Metall, Kupfergranulat, Aluminiumschrott, Stahlballen, Aluminiumballen, Kupferpaket, Wartungsset.

### Multiplayer und Sicherheit
- Alle Maschinenaktionen laufen über die bekannten Server-RPCs mit Reichweitenprüfung; Kundengeräte, Sammlerstücke und seltene Funde werden nie verarbeitet. Laufende oder befüllte Maschinen lassen sich nicht verschieben oder abbauen. Im Netzwerk mit 2 Spielern getestet (Stromgrenze, Produkte entnehmen).

### Technisch
- `UWEMachineDefinition` (Rezepte als Daten), `AWEMachine` (Eingang + Ausgabe, replizierter Status, Fortschritt lokal hochgerechnet), `UWEPowerGridComponent` (zentrale Simulation auf einem Server-Timer, kein Tick pro Maschine).
- Neue Interaktion [X] (`GetExtraInteraction`/`InteractExtra`) außerhalb des Baumodus.
- Speicherformat Version 8: Lager je Komponente, Maschinenzustand inkl. laufendem Vorgang, Stromzähler.
- Neues Skript `Tools/Maps/build_machines.py`; Dev-RPCs `ServerDevGiveItem`, `ServerDevAddMoney`.
- 8 neue Automation-Tests (insgesamt 75) inkl. Balance-Messung der Recyclingkette; Netzwerktest-Rollen Q/R; Screenshot-Lauf P9.

## [0.8.0] – 2026-10-14 – Phase 8: Werkstatt-Upgrades

### Neu
- Baumodus [B] (`UWEBuildComponent`): Vorschau am Fadenkreuz (grün/rot mit Grund), Kollisionsprüfung, Drehen ([R] 90°, [Q] 15°), Einrasten im 10-cm-Raster, Bestätigen mit [E], Auswahl per Mausrad.
- "Bearbeiten": Möbel anfassen und neu abstellen (auch die Grundausstattung), gekaufte Möbel mit [X] abbauen (halber Preis zurück); nicht während der Arbeit oder mit Gegenständen darauf.
- Möbelkatalog (`DA_FurnitureCatalog`): Werkbank, Große Werkbank (20 Plätze, 20 % schneller), Schwerlastregal, Großes Lagerregal (25 Plätze, 300 kg), Werkzeugwand, Lagerkiste, Ablagetisch.
- Mehrere Werkbänke; neue Werkbänke bekommen vorhandene Upgrades.
- Neue Werkzeuge: Bit-Ratsche, Profi-Seitenschneider, Digitalmultimeter, Lötstation; Sackkarre (+30 kg Tragkraft).
- Baumodus-Panel im HUD mit Katalog, Preisen und Hinweisen.

### Multiplayer und Sicherheit
- Aufstellen, Verschieben und Abbauen laufen über Server-RPCs; der Server prüft Baubereich, Platz, Geld und Anfrage-ID erneut. Gekaufte Möbel replizieren ihre Katalog-Id, Clients bauen Geometrie und Lager daraus. Im Netzwerk mit 2 Spielern getestet.

### Behoben
- Laden direkt nach dem Abbauen eines Möbels konnte dessen Namen (und damit den Lagerinhalt) nicht wiederherstellen.

### Technisch
- Speicherformat Version 7 (alle Möbel mit Name, Katalog-Id und Position; beim Laden zuerst).
- Neues Skript `Tools/Maps/build_workshop.py`; Map mit Baubereich (`AWEBuildArea`); Möbel sind beweglich (Movable) und replizieren ihre Bewegung.
- 3 neue Tastenbelegungen (B, X, Mausrad; insgesamt 20).
- 6 neue Automation-Tests (insgesamt 67); Netzwerktest-Rollen O/P; Screenshot-Lauf P8.

## [0.7.0] – 2026-10-14 – Phase 7: Erweiterte Beschaffung

### Neu
- Schrotthaufen hinter dem Tor (`AWEScrapPile`): bis zu 4 Gratis-Teile, Nachschub alle 8 Minuten, Hinweis beim Hinschauen.
- Wechselndes Händlerangebot: Jeder Nachschub würfelt, welche Angebote da sind und in welchem Zustand bzw. welcher Qualität (Chance und Streuung je Angebot im Datensatz).
- Retourenpaletten (Elektronik, Werkzeug) beim neuen Händler "Retouren-Großhandel Paletti"; an der Lieferpalette mit [Q] öffnen, Inhalt aus Beutetabellen (`UWELootTable`), pro Palette festgelegt.
- Auktionen (`UWEAuctionComponent`, `DA_AuctionCatalog` mit 7 Posten wie Büroauflösung, Werkstattauflösung, Sammlernachlass, Industrie-Servomotor): Mitbieter mit geheimem Limit, Verlängerung bei späten Geboten, Bezahlung erst beim Zuschlag, Lieferung auf die Palette.
- Neuer Reiter "Auktionen & Beschaffung" im Marktplatz mit laufenden Losen, Ergebnissen und Erklärungen zu allen Quellen.
- Seltene Gegenstände: Grafikkarte (Sammlerstück), Röhrenradio (1958), Keramik-Prozessor mit Goldkontakten, Industrie-Servomotor; Seltenheit wird beim Untersuchen und im Marktplatz angezeigt.
- Erweiterte Kundenaufträge: Mindestqualität, große Geräte in der Hand abliefern, Eilauftrag, Sammlerauftrag (4 neue Vorlagen).

### Multiplayer und Sicherheit
- Gebote laufen über den Server mit Reichweite und Anfrage-ID; die Firma kann nicht gegen sich selbst bieten; das geheime Limit bleibt auf dem Server. Im Netzwerk mit 2 Spielern getestet.

### Balancing
- Retourenpaletten 35,10 € / 29,90 €: direkt weiterverkauft weniger wert, mit Arbeit mehr; seltener Fund in 4–6 % der Paletten. Ein Schrotthaufen-Teil bringt direkt verkauft ~1 €.

### Technisch
- Speicherformat Version 6 (gewürfelter Händlervorrat, laufende Auktionen, Auftragsqualität).
- Neues Skript `Tools/Maps/build_procurement.py` (Beutetabellen); Map um den Schrotthaufen ergänzt.
- 8 neue Automation-Tests (insgesamt 61), darunter ein Balance-Test für Paletten und Schrott mit echten Daten; Netzwerktest-Rollen M/N; Screenshot-Lauf P7.
- Nur in Entwicklungs-Builds: `ServerDevEndAuctions` für die Testautomatisierung.

## [0.6.0] – 2026-10-13 – Phase 6: Reparatursystem

### Neu
- Reparieren an der Werkbank mit der neuen Taste [Q]: Fehler suchen (Multimeter) → defektes Teil ausbauen → Ersatzteil einbauen → Funktionstest → optional reinigen und aufbereiten.
- Verborgene Fehler je Gerät (`UWERepairDefinition`, Data Assets `REP_OldComputer`, `REP_CrtMonitor`): beschädigte Geräte haben einen, defekte einen oder zwei; erst die Diagnose zeigt sie.
- Ersatzteile aus dem Inventar oder dem Werkbanklager – gekauft oder aus einem anderen Gerät. Beschädigte Ersatzteile können den Funktionstest nicht bestehen.
- Qualitätsverbesserung: Reinigungsset hebt die Qualität um 30 %, ab 80 % wird ein Gerät "Sehr gut".
- Neue Gegenstände: Multimeter, Lötkolben, Kondensatoren-Set, Reinigungsset. Neuer Händler "Ersatzteil-Express" (gebrauchte Netzteile, RAM, Grafikkarte, Mainboard, Kondensatoren, Reinigungssets).
- Reparaturaufträge: Kunde bringt sein Gerät (erscheint an der Lieferpalette), Rückgabe repariert in der Hand; Kundengeräte können nicht verkauft oder zerlegt werden.
- Untersuchen [F] und Werkbank zeigen den Reparaturstand ("Netzteil defekt (ausgebaut)" …).

### Balancing
- Alter Computer 75 € Grundwert, kaputt ×0,42 (defekt beim Händler 11,36 €); Röhrenmonitor 30 €, kaputt ×0,5.
- Gemessen pro PC: Zerlegen +5,81 €, Reparieren +8,20 €, Reparieren und Aufbereiten +17,70 €.
- Beim Zerlegen kommen bekannte kaputte Teile als "Defekt" heraus; ein für die Reparatur ausgebautes Teil fehlt beim Zerlegen (keine Verdopplung).

### Behoben
- Marktplatz zeigte direkt nach einem Kauf für ein Bild übereinanderliegende Zeilen.

### Technisch
- Speicherformat Version 5 (Reparaturstand jedes Gegenstands, Reparaturaufträge).
- Interaktions-Schnittstelle um eine dritte Aktion ([Q]) erweitert; 17 Tastenbelegungen.
- 7 neue Automation-Tests (insgesamt 53), darunter ein Balance-Test für Reparatur mit echten Spieldaten; Netzwerktest-Rollen K/L (Reparaturauftrag mit 2 Spielern); Screenshot-Lauf P6.
- Nur in Entwicklungs-Builds: `ServerDevAddContractOffer` für die Testautomatisierung.

## [0.5.0] – 2026-10-13 – Phase 5: Erster vollständiger Gameplay-Loop (Vertical Slice)

### Neu
- Werkstatt-Upgrades als Data Asset (`DA_UpgradeCatalog`, 6 Stück): LED-Beleuchtung, Werkbank-Unterschrank, zweites Regal, Werkzeuggürtel, Arbeitsmatte, Profi-Werkbank – teils mit Voraussetzung.
- Wirkungen: mehr Werkbankplätze, mehr Tragkraft für alle Spieler, schnelleres Zerlegen, hellere Lampen, das zweite Regal erscheint erst nach dem Kauf.
- Marktplatz: neue Spalte "Werkstatt verbessern" (gekauft / Voraussetzung / Kaufen).
- Leitfaden oben links: "Erste Schritte (1/6)" führt durch den ganzen Ablauf, danach "Nächstes Ziel: …".
- Knopf "Alles nehmen" in Lagern (z. B. alle Teile aus der Werkbank ins Inventar).

### Multiplayer und Sicherheit
- Upgrades kauft nur der Server (Reichweite, Voraussetzung, Doppelkauf, Geld, Anfrage-ID); sie gelten für das ganze Team und werden an alle repliziert. Im Koop als Listen-Server getestet.

### Behoben
- Gekaufte Werkzeuge zählten nicht für den ersten Leitfaden-Schritt.

### Technisch
- Speicherformat Version 4 (Upgrades, Leitfaden); ältere Spielstände bleiben ladbar.
- Neues Skript `Tools/Maps/build_progression.py`; Map um zweites Regal und markierte Werkstattlampen ergänzt.
- 5 neue Automation-Tests (insgesamt 46), darunter der komplette Ablauf mit echten Spieldaten; Netzwerktest-Rollen I/J (Listen-Server); Screenshot-Lauf P5 mit dem ganzen Ablauf.

## [0.4.0] – 2026-10-12 – Phase 4: Wirtschaft und Handel

### Neu
- Firmenkonto (`UWECompanyComponent`): 250 € Startkapital (Config), Cent-genau, nie negativ, letzte Buchungen; Anzeige oben rechts im HUD.
- Markt (`UWEMarketComponent`): Nachfrage je Kategorie (0,8–1,2) mit langsamer Schwankung; Preise aus Wert, Zustand, Qualität, Nachfrage und Händlerfaktor.
- 3 Händler als Data Assets: Schrotthandel Kowalski, Gebrauchtbörse 24, Werkzeug-Versand Hammerl – mit Ankaufskategorien, Angebot und Nachschub.
- Online-Marktplatz-Terminal in der Garage ([E]): verkaufen aus dem Inventar oder den getragenen Gegenstand (an den besten Bieter), kaufen (Kleinteile ins Inventar, Großes auf die neue Lieferpalette am Tor).
- Kundenaufträge (`UWEContractComponent`, Auftragskatalog mit 7 Vorlagen): 3 Angebote, annehmen, abliefern mit Mindestzustand, Fristen, Belohnung aufs Firmenkonto.
- Handelsbildschirm mit Spalten Verkaufen / Kaufen / Kundenaufträge und letzten Buchungen.

### Multiplayer und Sicherheit
- Alle Käufe, Verkäufe und Auftragsaktionen laufen über Server-RPCs mit Reichweiten-, Zugriffs-, Geld- und Vorratsprüfung.
- Schutz vor Doppelbuchungen: jede Anfrage trägt eine eindeutige ID, Wiederholungen werden ignoriert (im Netzwerk getestet).
- Konto, Nachfrage, Aufträge und Vorrat werden an alle Spieler repliziert.

### Balancing
- Zustandsfaktoren angepasst; Teile aus defekten Geräten kommen gleich gut oder besser heraus. Gemessen: defekten PC kaufen, zerlegen, verkaufen ≈ +6 € Gewinn.

### Technisch
- Speicherformat Version 3 (Konto, Markt, Aufträge, Händlervorrat).
- Neues Skript `Tools/Maps/build_economy.py`; Map um Terminal und Lieferpalette ergänzt.
- 8 neue Automation-Tests (insgesamt 41), darunter ein Balance-Test mit echten Spieldaten; Netzwerktest-Rollen G/H; Screenshot-Lauf P4.
- Nur in Entwicklungs-Builds: Teleport-RPC und Doppelsende-Hilfe für die Testautomatisierung.

## [0.3.0] – 2026-10-12 – Phase 3: Werkbank und Zerlegung

### Neu
- Werkbank `AWEWorkbench`: sperrigen Gegenstand mit [E] auf den Arbeitsplatz legen, mit [R] wieder herunternehmen; [E] auf der Werkbank öffnet weiterhin das Werkbanklager (jetzt 12 Plätze).
- Schrittweises Zerlegen: [E] auf den Gegenstand startet den nächsten Schritt; Hinweis zeigt Aktion und Werkzeug, z. B. "Gehäuseschrauben lösen (Schraubendreher)".
- Wiederverwendbare Zerlegeabläufe als Data Assets (`UWEDisassemblyDefinition`): Alter Computer (7 Schritte), Röhrenmonitor (4), Karton mit Kabeln (1).
- Werkzeuge mit Typ und Tempo: Schraubendreher, Seitenschneider (neu in der Garage), Akkuschrauber (×2,5, definiert für später).
- 9 neue Gegenstände: Grafikkarte, PC-Netzteil, Mainboard, Stahlschrott, Kunststoffteile, Glasbruch, Elektronikschrott, Seitenschneider, Akkuschrauber.
- Komponenten erben den Zustand der Quelle (±1 Stufe), Rohstoffe stapeln; Ergebnisse landen im Werkbanklager, Überschuss vor der Werkbank.
- Feedback: Fortschrittsbalken im HUD, Fortschrittsleiste an der Werkbank (orange/grün), Meldung mit den gewonnenen Teilen, 5 synthetisch erzeugte Sounds (Ablegen, Schrauben, Teil ab, fertig, Abbruch).
- Untersuchen zeigt Zerlegefortschritt und Werkzeugtempo; teilweise zerlegte Gegenstände heißen "… (teilweise zerlegt)".
- Neue Taste R für Zweitaktionen.

### Multiplayer
- Platzieren, Starten und Abschließen von Schritten laufen auf dem Server; ein Schritt gleichzeitig pro Werkbank; wer sich entfernt, bekommt kein Ergebnis.
- Repliziert werden nur der abgelegte Gegenstand und ein kleiner Arbeitszustand; Fortschritt, Sounds und Anzeige berechnet jeder Rechner selbst.

### Technisch
- Speicherformat Version 2: Zerlegefortschritt und Gegenstand auf der Werkbank; Version-1-Stände bleiben lesbar.
- Neues Skript `Tools/Maps/build_audio.py`; Item-Skript legt jetzt auch Zerlegeabläufe an; Map nutzt die neue Werkbank und ein angepasstes Layout.
- 7 neue Automation-Tests (insgesamt 33), Netzwerktest-Rollen E/F, Screenshot-Lauf P3.

### Fehlerbehebungen (während der Phase)
- Kein Hinweis beim Anschauen eines Gegenstands auf der Werkbank (Kollision war aus).
- Lose Gegenstände auf der Werkbank verhinderten das Ablegen.
- Fortschrittsbalken bei anderen Spielern sprang durch abweichende Serverzeit zu früh auf 100 %.
- Doppelte Namen in der Teile-Meldung, schlecht lesbarer Text auf hellen Inventarkacheln, Knackser in den Sounds.

## [0.2.0] – 2026-10-12 – Phase 2: Items und Inventar

### Neu
- Datengetriebenes Itemsystem: `UWEItemDefinition` (Data Asset) mit Kategorie, Seltenheit, Gewicht, Grundwert, Stapelbarkeit, Inventartauglichkeit, Reparier-/Zerlegbarkeit, Materialzusammensetzung, Mesh, Farbe und Icon-Feld.
- 9 Gegenstände als Data Assets: Alter Computer, Röhrenmonitor, Schraubendreher, Werkzeugkiste, Alter Elektromotor, Karton mit Kabeln, Blechschrauben, Kupferkabel, Arbeitsspeicher.
- Item-Instanzen `FWEItemInstance`: eindeutige ID, Menge, Zustand (Neu … Schrott), Qualität, Schätzwert.
- Inventarsystem `UWEInventoryComponent`: Plätze, Gewichtsgrenze, Stapeln, Teilen, Zusammenführen, Tauschen, Sortieren, Transfer zwischen Inventaren.
- Spielerinventar (20 Plätze, 25 kg), Werkbanklager (8/60 kg), Regallager (15/150 kg); [E] öffnet Werkbank/Regal.
- [E] steckt Kleinteile ein (auch teilweise, wenn das Gewicht nicht für alles reicht); sperrige Gegenstände werden weiter getragen.
- Inventarfenster (Tab/I) mit Drag & Drop, Shift = halber Stapel, Doppelklick = umlagern, Rechtsklick = ablegen, Tooltips mit allen Werten.
- Untersuchen zeigt jetzt Kategorie, Zustand, Qualität, Gewicht und Schätzwert.
- Speichern und Laden: F5/F9, benannte Spielstände (`WESave`/`WELoad`), Autosave alle 5 Minuten, Kurzmeldungen im HUD.

### Multiplayer
- Inventare per Fast-Array-Replikation (nur Änderungen werden übertragen).
- Alle Inventaraktionen laufen über Server-RPCs; der Server prüft Besitz und Reichweite. Fremde Spielerinventare sind gesperrt.
- Getestet: keine Duplikate, Mengen bleiben über Spieler, Lager und Welt erhalten.
- Speichern/Laden nur durch den Host; später beitretende Spieler bekommen ihren gespeicherten Stand.

### Technisch
- Spielstandformat mit Versionsnummer, CRC-Prüfung, `.tmp`-Schreiben und `.bak`-Sicherung; beschädigte Dateien fallen auf das Backup zurück.
- Gegenstände werden über stabile `ItemId` gespeichert; Asset Manager kennt den Typ `WEItem`.
- Neues Skript `Tools/Maps/build_item_definitions.py`; Map-Skript nutzt die Definitionen und richtet die Lager ein.
- `AWEPickupItem` stellt jetzt eine Item-Instanz dar (vorher eigene Name/Gewicht-Felder).
- 12 neue Automation-Tests (insgesamt 26), Netzwerktest-Rollen C/D, Screenshot-Lauf P2 mit Speichern/Laden.

### Fehlerbehebungen (während der Phase)
- Fehlende Modulabhängigkeit `CoreOnline`.
- Asset-Manager-Fehler durch abweichende Primary Asset Ids.
- Lange Namen liefen in Nachbarkacheln; Welt-Hinweise schienen durch das geöffnete Inventar.

## [0.1.0] – 2026-10-12 – Phase 1: Spielbarer First-Person-Prototyp

### Neu
- First-Person-Charakter `AWECharacter`: Gehen, Sprinten (Shift), Ducken (Strg/C) mit weicher Kamera, Springen, Mausblick; Empfindlichkeit über `we.MouseSensitivity`, Invertieren über `we.InvertY`.
- Eingabe über Enhanced Input (`UWEInputConfig`), Standardbelegung WASD/Maus/Leertaste/Shift/Strg/C/E/F.
- Interaktionssystem: `IWEInteractable` + `UWEInteractionComponent` mit Hinweisen "[E] Aufheben", "[E] Ablegen", "[E] Tor öffnen", "[F] Untersuchen".
- Gegenstände `AWEPickupItem`: aufnehmen, tragen (max. 30 kg), dort ablegen, wo man hinschaut, untersuchen (Name, Beschreibung, Gewicht).
- Garagentor `AWEGarageDoor` zum Öffnen/Schließen.
- HUD `UWEHUDWidget`: Fadenkreuz, Hinweise, "In der Hand", Untersuchen-Panel.
- Platzhalter-Garage `L_Garage` (neue Startmap) mit Werkbank, Regal und 6 Gegenständen (Computer, Röhrenmonitor, Schraubendreher, Werkzeugkiste, Karton mit Kabeln, zu schwerer Elektromotor).

### Multiplayer
- Alle Interaktionen laufen als Server-RPC; Server prüft Reichweite und Regeln.
- Zwei Spieler können nicht denselben Gegenstand nehmen; verlässt ein Spieler das Spiel, fällt sein Gegenstand in die Welt.
- Getragene Gegenstände und Torzustand werden repliziert.

### Technisch
- Neue Ordner `Player/`, `Interaction/`, `Items/`, `World/`, `UI/`, `Dev/` im Spielmodul; UMG/Slate als Abhängigkeit.
- Platzhalter-Material `M_WEPlaceholder` (Farbparameter, für Instanced Meshes freigegeben) und `MI_WEGround`.
- Reproduzierbare Erzeugung von Material und Map: `Tools/Maps/build_placeholder_material.py`, `Tools/Maps/build_garage_map.py`.
- Test-Automatisierung `UWEDevAutomationComponent` (nur Nicht-Shipping): `-WESmokeRole=A|B`, `-WEShots=<Name>`.
- 9 neue Automation-Tests (insgesamt 14), Test-Welt-Harness `FWETestWorld`.
- Plugin Python Editor Script (nur Editor).

### Fehlerbehebungen (während der Phase)
- Test-Welt startete kein BeginPlay/EndPlay, dadurch war "Ablegen beim Verlassen" ungetestet.
- Platzhalter-Farben wurden im Spiel nicht angezeigt (fehlender Materialparameter bzw. Instanced-Mesh-Flag).
- Überbelichtete Garage; große getragene Gegenstände verdeckten die Sicht.

## [0.0.1] – 2026-10-12 – Phase 0: Projektanalyse und Setup

### Neu
- UE 5.7 C++-Projekt `WorkshopEmpire` mit Editor- und Game-Target.
- Modul `WECore`: `FWEMoney` (Geld in int64 Cent, `TryAdd`/`TrySubtract` mit Überlaufprüfung, Anzeige "1.234,56 €"), Log-Kategorien `LogWorkshopEmpire` und `LogWENet`.
- Modul `WorkshopEmpire`: `AWEGameMode` (Koop-Limit 4 Spieler per Config, Ablehnung weiterer Logins, Join/Leave-Log), `AWEGameState`, `AWEPlayerState`, `AWEPlayerController` (NetMode-Log).
- Modul `WorkshopEmpireTests`: 5 Automation-Tests (`WorkshopEmpire.Core.Money.*`, `WorkshopEmpire.Framework.GameModeDefaults`).
- Projektdokumentation: PROJECT_PROGRESS, GAME_DESIGN, TECHNICAL_ARCHITECTURE, CHANGELOG, TEST_REPORT; Spezifikation unter `Docs/MASTER_PROMPT_V1.0.md`.

### Technisch
- `AWEGameMode` als globaler Standard-GameMode, Platzhalter-Map `/Engine/Maps/Templates/Template_Default`.
- DX12, SM6, Lumen und Virtual Shadow Maps (Engine-Standard), keine statische Beleuchtung.
- Plugins: Enhanced Input, Modeling Tools Editor Mode (nur Editor, für Platzhalter-Geometrie).
- Git-Repository mit Unreal-`.gitignore` und `.gitattributes`.
