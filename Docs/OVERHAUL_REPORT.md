# Massive Quality Overhaul – Abschlussbericht

Auftrag vom 2026-10-24. Sicherungspunkt vor Beginn: Git-Tag `checkpoint-before-overhaul` (a92a7b1).
Commits: bb70270, adc85c7, a4c7266, de8c5b6, c33ad7a, 9db342e, d30bdbd (+ Doku). Arbeitsliste mit allen Befunden: `Docs/OVERHAUL_PROGRESS.md`.

Das Spiel ist damit **nicht fehlerfrei**. Dieser Bericht trennt, was automatisch geprüft wurde, von dem, was nur ein Mensch beurteilen kann (Abschnitt E).

## A. Behobene Fehler

| Schwere | Fehler | Behebung |
|---|---|---|
| CRITICAL | Absturz beim Kauf eines einzelnen Teils, das per Paketdienst kommt (z. B. das Multimeter im Tutorial): Array-Grenzfehler in `UWECourierComponent::Order` | Grenzprüfung; Test 1 kauft jetzt das Multimeter ohne Absturz |
| HIGH | Deko ohne gewollte Kollision (Besen, Kanister, Dosen, Steckdosen, Hallen-Deko …) blockierte Maschinenplätze und zerschnitt das Navmesh der Mitarbeiter (z. B. Presse an der Ostwand: „Kein Platz“) | Kollision der Deko wirklich aus (Profil `NoCollision`) |
| MEDIUM | Testablauf: eine geplante Aktion lief nach dem Wechsel ins Hauptmenü noch auf dem alten Spieler-Controller → Absturz des P13-Clients | geplante Aktionen nur, solange ihr Besitzer lebt; Pausenmenü ohne lokalen Spieler wird nicht geöffnet |
| HIGH | Angenommener Auftrag ließ sich nicht abgeben – blockierte bis zum Ablauf, Gerät verloren | „Abgeben“ mit Rückfrage, Kunde holt sein Gerät ab |
| HIGH | Mitarbeiter blieben dauerhaft auf „kein Weg frei (Tor zu?)“ hängen (Start knapp neben dem Navmesh, nur eine Möbelseite als Ziel, Kaffee-Ziele hinter der Wand) | Start vom nächsten Navmesh-Punkt, alle vier Seiten, Ziele hinter Wänden verworfen |
| MEDIUM | Sicherungskasten quer und 20 cm in der Wand | gedreht, bündig, Leerrohr zur Decke |
| MEDIUM | PC-Tischlampe: Gelenk in der Wand (sah zerbrochen aus), Schreibtisch 10 cm in der Wand | Tisch vorgezogen, Lampe gedreht |
| MEDIUM | Besen, Schaltschrank, Feuerlöscher, Leiter, Karton steckten in den Garagenwänden | per Audit gefunden und aus den Wänden geschoben |
| MEDIUM | Untersuchen-Fenster: Zeilen übereinander bei laufenden Maschinen | Zeilen werden wiederverwendet statt jedes Bild neu gebaut |
| MEDIUM | Maschinenbediener stand bis 1,4 m vor der Maschine und griff ins Leere | tritt an die Bedienseite, Arme auf dem Bedienfeld |
| MEDIUM | Kein Geräusch beim Aufnehmen, Fallenlassen, Einstecken, Garagentor | Geräusche nach Material, Gewicht und Aufprall |
| LOW | Seitenschneider fiel von der Werkbank | liegt flach auf der Platte |
| LOW | Maschinenstatus lief unter die Prozentzahl | Zeilenumbruch |
| LOW | Sitzende Mitarbeiterin: Knie in der Schublade bzw. Blick in die Wandecke | Blickrichtung zu Tisch oder freiem Raum, Hocker versetzt |
| LOW | Ampeln: alle drei Lichter gleichzeitig an | je Richtung nur Rot oder Grün |
| LOW | Zwei Hydranten nebeneinander (Datei mit zwei Varianten) | nur eine Variante |
| LOW | Firmenname auf dem weißen Schild gelb auf weiß | dunkle Schrift |
| LOW | Aus der Garage in den Hof rund 2 s weiß überstrahlt | schnellere Belichtungsanpassung |

## B. Ersetzte Objekte (Platzhalter → Modell)

| Vorher | Nachher |
|---|---|
| Werkbank und große Werkbank aus Sperrholzkisten | Werkbank mit Stahlgestell, Holzplatte, Schubladen, Ablage |
| Deckenleuchten = flache Kisten mit leuchtender Kiste | Doppelrohr-Leuchte mit leuchtenden Röhren |
| Lieferpalette = OSB-Quader | Palettenmodell |
| 7 Maschinen, 2 Förderbänder, 2 Gitterboxen = lackierte Quader | Modelle mit Bedienfeld, Signalsäule, Motor, Trichter … |
| Schrotthaufen im Hof = gekippte Kisten | Schrottberg (Bleche, Reifen, Waschmaschine, Heizkörper, Rohre) |
| Erdhaufen am Kartenrand = gedrehte Quader | Erdhügel mit Steinen (beide Karten) |
| Kaffeebecher in der Hand = Engine-Zylinder | Becher mit Henkel und Kaffee |
| Ampelkopf = schwarze Kiste mit drei Scheiben | Ampelmodell mit Blenden und Rückplatte |
| leere Garage und Hallen | Steckdosen, Schalter, Leerrohre, Abzweigdosen, Uhr, Dartscheibe, Schlauchtrommel, Schubladenschrank, Bodenmarkierung, Stromkästen, Feuerlöscher, Paletten, Regale, Fässer, Wandlampen |

Alle neuen Modelle sind selbst erzeugt (Geometry Script) oder CC0 von Poly Haven (`Docs/ASSET_SOURCES.md`) – kommerziell nutzbar.
Bewusst nicht ersetzt (als Platzhalter markiert): die Stadtgebäude im Hintergrund sind texturierte Kästen; Masten und Ausleger der Ampeln und Laternen sind Engine-Zylinder.

## C. Verbesserte Mechaniken

- Aufträge: angenommene Aufträge abgeben (Strafe, Bewertung, Gerät verschwindet aus allen Inventaren).
- Mitarbeiter: Wegsuche robust (siehe A), Bediener an der Bedienseite, „fast da und blockiert“ zählt als angekommen, Sitzrichtung.
- Geräusche: Aufnehmen, Aufprall nach Material/Gewicht/Wucht (gedrosselt), Einstecken, Garagentor (Start, Ende offen/zu).
- Licht: schnellere Augen-Anpassung zwischen Garage und Hof.

## D. Tatsächlich getestete Abläufe

Automatisch, mit normalen Spieleraktionen (Server-Requests wie bei Eingaben):
- **Test 1** (`-WEShots=QUT`): neues Spiel mit Tutorial → alle 11 Tutorial-Schritte (Laufen, Werkzeug, Inventar, PC auf die Werkbank, Arbeitsansicht, Zerlegen, Werkstatt-PC, Verkaufen, Auftrag annehmen, Multimeter kaufen → Paket abwarten → öffnen → prüfen, Speichern) → Hauptmenü → Fortsetzen: Kontostand, Tutorialstand, Inventar identisch.
- **Kundenauftrag komplett** (`QUB`): annehmen, Gerät in die Hände, Werkbank, Seitenteil/GPU/Kühler aus- und einbauen inkl. Wärmeleitpaste, **Speichern und Laden mit halb zerlegtem Kunden-PC** (gleiche ID, gleicher Zerlegungsstand, Auftrag verknüpft), Funktionstest, Abliefern, Geld – im Editor und im gepackten Build.
- Tragen/Inventar/Platzierungsvorschau/Speichern (`QUA`), Garagen-Ansichten (`QUE`), Mitarbeiter (`P21E`), Maschinen und Linien (`P10`, `P12`), Netzwerk (`P13`, Paare A–X, 4 Spieler), UI 1280×720 bis 2560×1440 (`P17UI`), 140 Unit-Tests.
- **Volle Regression** (Editor, 25.10. nachts): P5–P16, P175W/V, P18R, P19G, P20, P21 (B, C, F, G, W, L), QUA–QUE, P19C, P21E, P21M, QUT: alle beendet, 0 Fehler. Netzwerk-Paare A/B bis W/X: alle Schritte wie erwartet. 4 Spieler (Beitreten, Verlassen, Wiederkommen mit Inventar, Absturz eines Clients, Kick, voller Server): wie erwartet. UI 1280×720, 1024×768, 1920×1080, 2560×1440: je 17/17. P13-Client stürzte im ersten Lauf ab (siehe A) – nach der Korrektur zweimal grün.
- Gepackter Build: Test 1 (QUT) bestanden, QUB/QUA/QUE bestanden.
- Vorher/Nachher-Bilder: `Docs/Overhaul_Vorher_Nachher.jpg` (Vorher = gepackter Build vom 20.10., gleiche Kamerapositionen).

## E. Verbleibende Probleme und manuelle Tests

Bekannt, nicht behoben:
- Straßenansicht kostet im Editor in 1080p rund 15 ms GPU (Lumen ~4,5 ms, TSR 2,1 ms, Volumenwolken 1,4 ms). Nicht angepasst, um das Aussehen nicht ungeprüft zu verschlechtern.
- Stadtgebäude im Hintergrund sind texturierte Kästen.
- Nach harten Kameraschnitten (nur in Testbildern) kurz Lumen-Rauschen an Wänden neben Figuren.
- 4K-UI nur rechnerisch geprüft (Monitor kann kein 4K-Fenster).
- Im Testbild P15 lässt sich der Schredder links neben dem PC-Tisch nicht mehr aufstellen (der Tisch steht seit dem Overhaul 12 cm von der Wand ab); an anderer Stelle geht es.
- Im Tutorial bringt „Verkaufe ausgebaute Teile“ nach dem ersten Zerlegeschritt nur wenige Cent (es liegen nur Schrauben in der Ablage).

Nur von Hand prüfbar (nicht als bestanden markiert):
1. Geräusche anhören: Lautstärke und Passung beim Aufnehmen, Fallenlassen auf Holz/Metall/Beton, Garagentor.
2. Arbeitsansicht mit Maus: Schrauben lösen und Teile ziehen von Hand (gefühlt, nicht nur per Funktion).
3. Mitarbeiter über längere Zeit beobachten (mehrere Pausen, volle Halle).
4. UI in echtem 4K.
5. Steam-Mehrspieler über zwei echte Konten (Steam-App-ID fehlt noch).

## F. Leistung

- Unit-Tests 140/140; Editor-Messungen sind ungenau, weil gleichzeitig Editoren anderer Projekte liefen.
- Straße (1080p, Editor): ~15 ms GPU, Aufteilung siehe E. Autos 0,5–8 ms je nach Ansicht (3 LOD-Stufen eingebaut).
- Untersuchen-Fenster baut Zeilen nicht mehr jedes Bild neu (vorher Widget-Neubau pro Bild bei laufenden Maschinen).
- Neue Modelle ohne Nanite, mit Box-Kollision; Deko ohne Kollision.

## G. Technische Änderungen

- Speicherstand-Version 20 (Quality Update); Overhaul ohne neues Speicherformat.
- Neue Werkzeuge: `Tools/Maps/audit_maps.py`, `garage_overhaul.py`, `patch_world.py`; `Tools/Assets/build_workshop_models.py`, `build_machine_models.py`, `import_variants.py`.
- Neue Testläufe: `QUT` (Test 1), `QUE`, `QUM`; `QUB` mit Speichern/Laden mitten in der Reparatur; `P21E` mit Navmesh-Ansicht.
- C++: `WEEmployee` (Wegsuche, Bedienplatz, Sitzrichtung), `WECourierComponent` (Absturz), `WEContractComponent`/`WETradeWidget` (Abgeben), `WEPickupItem`/`WEGarageDoor`/`WEInteractionComponent` (Geräusche), `WEHUDWidget` (Untersuchen), `WELogoComponent` (Schildschrift), `WEDayNightComponent` (Belichtung), `WETraderTerminal`/`WEScrapPile` (Modelle).
