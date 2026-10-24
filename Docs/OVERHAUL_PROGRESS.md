# Massive Quality Overhaul – Arbeitsstand

Auftrag: Mikes „MASSIVE QUALITY OVERHAUL“ vom 2026-10-24. Sicherungspunkt vor Beginn: Git-Tag `checkpoint-before-overhaul` (a92a7b1).
Diese Datei wird nach jedem Abschnitt fortgeschrieben, damit die Arbeit ohne Informationsverlust weitergehen kann.

## Werkzeuge
- `Tools/Maps/audit_maps.py` → `Saved/Audit/<Map>.txt`: Grundformen, fehlende Materialien, Platzhalter-Material, Objekte in den Garagenwänden, unter dem Boden, simulierte Physik.
- `Tools/Maps/garage_overhaul.py`: Korrekturen und Ausstattung der Startgarage und der Hallen direkt in L_Garage (idempotent, mehrfach ausführbar).
- `Tools/Assets/build_workshop_models.py`: Werkbank, große Werkbank, Steckdose, Schalter, Leerrohr, Abzweigdose, Doppelrohr-Leuchte.
- `Tools/Assets/build_machine_models.py`: 7 Maschinen, 2 Förderbänder, 2 Gitterboxen.
- Testläufe: `-WEShots=QUE` (Garagenansichten), `QUA` (Tragen/Speichern), `QUB` (kompletter Auftrag inkl. Speichern mit halb zerlegtem PC), `QUC`/`QUD` (Autos, `-WEProfileGPU`), `QUM` (Maschinenmodelle).

## Erledigt (mit Prüfung)
| # | Befund | Schwere | Behebung | Geprüft |
|---|---|---|---|---|
| 1 | Sicherungskasten quer, 20 cm in der Wand | MEDIUM | gedreht, bündig, Leerrohr | QUE-Bilder, Audit 0 Funde |
| 2 | PC-Lampe: Gelenk in der Wand (sah zerbrochen aus), Tisch 10 cm in der Wand | MEDIUM | Tisch 12 cm vor, Lampe gedreht | QUE-Bilder |
| 3 | Besen 31 cm, Schaltschrank 24 cm, Feuerlöscher, Leiter, Karton in Wänden | MEDIUM | aus den Wänden geschoben | Audit |
| 4 | Seitenschneider fiel von der Werkbank | LOW | liegt flach auf der Platte | QUE |
| 5 | Werkbank aus Sperrholzkisten, Deckenleuchten = flache Kisten, Lieferpalette = OSB-Quader | HIGH (Platzhalter) | Modelle | QUE, QUA, QUB |
| 6 | Maschinen, Förderbänder, Materiallager = lackierte Quader | HIGH (Platzhalter) | Modelle | P10, P12, QUM |
| 7 | Untersuchen-Fenster: Zeilen übereinander bei laufenden Maschinen | MEDIUM | Zeilen werden wiederverwendet | P12 |
| 8 | Maschinenstatus lief unter die Prozentzahl | LOW | Zeilenumbruch | P12 |
| 9 | Angenommener Auftrag ließ sich nicht abgeben (blockierte bis Ablauf, Gerät verloren = festgefahren) | HIGH | „Abgeben“ mit Rückfrage | Unit-Test CancelAcceptedOrder |
| 10 | Garage und Hallen leer/steril | MEDIUM | Elektroinstallation, Deko, Hallen-Ausstattung | QUE, P12 |
| 11 | Schrotthaufen im Hof = gekippte Kisten („placeholder“ im Code) | HIGH (Platzhalter) | Modell, Kisten nur noch Kollision | QUE_09 |
| 12 | Erdhaufen am Kartenrand = gedrehte Quader | MEDIUM | Hügel-Modell (beide Karten) | patch_world.py |
| 13 | Kein Geräusch beim Aufnehmen, Fallen/Ablegen, Einstecken, Garagentor | MEDIUM | Geräusche nach Material, Gewicht und Wucht | Tests grün, im Spiel nicht angehört |
| 14 | Untersuchen-Fenster/Maschinenstatus (siehe 7, 8) | – | – | – |

Tests: 140/140 Unit-Tests; QUA, QUB (auch gepackt), QUE (auch gepackt), P10, P12, P17UI 1080p.

## Offen (Reihenfolge)
- Außenbereich/Straße: Leistung (Straßenansicht ~15 ms GPU in 1080p im Editor; TSR 2,1 ms, Wolken 1,4 ms, Lumen ~4,5 ms), Fassaden prüfen.
- Hauptmenü-Kulisse (eigene Szene in L_MainMenu) prüfen.
- Sounds und Animationen gezielt durchgehen; Mitarbeiter-Posen an Maschinen.
- UI in 4K wirklich ansehen (Monitor kann kein 4K-Fenster; rechnerisch über die DPI-Kurve gleich 1080p).
