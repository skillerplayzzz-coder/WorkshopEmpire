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
| 14 | Maschinenbediener stand bis 1,4 m vor der Maschine und griff mit ausgestreckten Armen ins Leere | MEDIUM | tritt an die Bedienseite, schaut sie an, Arme auf das Bedienfeld | P21E |
| 15 | Mitarbeiter blieben dauerhaft auf „kein Weg frei (Tor zu?)“ hängen: Start knapp neben dem Navmesh (direkt an Maschinen, zwischen Kram), nur eine Möbelseite als Ziel, Kaffee-Ziele hinter der Wand verdrängten erreichbare | HIGH (blockiert Arbeit) | Start vom nächsten Navmesh-Punkt bzw. Punkten rundherum, alle vier Seiten, Ziele hinter Wänden verworfen, „fast da und blockiert“ = angekommen | P21E (Log: 0× kein Weg frei), Unit-Tests |
| 16 | Sitzende Mitarbeiterin: Knie in der Schubladeneinheit bzw. Blick in die Wandecke | LOW | Blickrichtung nach Tisch/freiem Raum, Hocker verschoben | P21E |
| 17 | Kaffeebecher in der Hand = skalierter Engine-Zylinder | LOW (Platzhalter) | Becher-Modell mit Henkel und Kaffee | P21E |
| 18 | **Absturz**: ein einzelnes Teil im Shop kaufen, das per Paketdienst kommt (z. B. Multimeter im Tutorial) → Array-Grenzfehler in `UWECourierComponent::Order` (Development-Build stürzt ab) | CRITICAL | Grenzprüfung | Test 1 (QUT): Kauf ohne Absturz |
| 19 | Ampeln: schwarze Kiste mit drei flachen Zylindern, alle drei Lichter gleichzeitig an | MEDIUM (Platzhalter) | Ampel-Modell (Gehäuse, Blenden, Rückplatte), je Richtung nur Rot oder Grün | QUE_10, beide Karten (8 Ampeln) |
| 20 | Zwei Hydranten nebeneinander (Poly-Haven-Datei enthält zwei Varianten) | LOW | nur die gealterte Variante | P12_04 |
| 21 | Firmenname auf dem weißen Schild gelb auf weiß, kaum lesbar | LOW | dunkle Schrift auf hellem Schild | P12_04 |
| 22 | Aus der Garage in den Hof: rund 2 s weiß überstrahlt | LOW | Belichtungsanpassung schneller (5 / 2 EV/s) | QUE_09 |
| 23 | Test 1 fehlte (neues Spiel → Tutorial → Geld → Speichern → Menü → Fortsetzen) | – | automatischer Durchlauf `-WEShots=QUT` auf L_MainMenu, nur normale Spieleraktionen | siehe Testbericht |

Tests: 140/140 Unit-Tests; QUA, QUB (auch gepackt), QUE (auch gepackt), P10, P12, P13, P21E, P17UI 1080p.

## Offen (Reihenfolge)
- Außenbereich/Straße: Leistung (Straßenansicht ~15 ms GPU in 1080p im Editor; TSR 2,1 ms, Wolken 1,4 ms, Lumen ~4,5 ms), Fassaden prüfen.
- Hauptmenü-Kulisse (eigene Szene in L_MainMenu) prüfen.
- Sounds und Animationen gezielt durchgehen (Mitarbeiter-Posen erledigt, siehe 14–16).
- UI in 4K wirklich ansehen (Monitor kann kein 4K-Fenster; rechnerisch über die DPI-Kurve gleich 1080p).
