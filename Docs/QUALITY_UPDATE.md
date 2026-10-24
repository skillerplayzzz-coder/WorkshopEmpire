# Quality Update (0.22.0): PC-System, Kundenaufträge, Autos

Stand: 2026-10-24. Grundlage: Mikes Auftrag „QUALITY UPDATE – Vollständige Überarbeitung des PC-Systems, der Kundenaufträge und der Außenwelt“.

## 1. Analyse (vorher)

| Bereich | Befund |
|---|---|
| Auftrag → Palette | Der Büro-PC wurde schon getragen (nicht ins Inventar). **Kleine Kundengeräte** (Kofferradio, Konsole, Akkuschrauber) passen ins Inventar: [E] steckte sie ein, und aus dem Inventar gab es **keinen Weg auf die Werkbank** (Ablegen → [E] steckte sie wieder ein). |
| Kunden-PC auf der Werkbank | „Kundengerät – nicht zerlegen“: nur der abstrakte [Q]-Reparaturablauf, kein Öffnen, kein Teiletausch (nur Aufrüst-Aufträge durften Teile tauschen). |
| Arbeitsmodus | Schon vorhanden: Orbit, Zoom zum Zeiger, Hover-Umriss, Schrauben/Stecker/Klammern/Hebel je Teil in fester Reihenfolge, Voraussetzungen, Schraubenschale, Werkzeug in der Hand, Ersatzteil aus dem Karton, Funktionstest. Fehlte: Kamera konnte ins Gehäuse und durch Regale, RAM als ein Teil, Wärmeleitpaste nur als Wartung, kein leeres Gehäuse zum Neubau, ein Dutzend Teileschilder gleichzeitig. |
| Autos | Kenney Car Kit (Spielzeug-Low-Poly, 2,6 m lang, hochskaliert). |
| Geometrie-Bibliothek | `build_models.py`: der Torus übernahm die Material-ID nicht (alle Torus-Dreiecke bekamen Material 0). |

## 2. Was jetzt geht

### Kundenaufträge (Phase 2)
- Kundengeräte kommen mit [E] **immer in die Hand** („E | PC aufnehmen“), nie ins Inventar.
- Beim Tragen: **grüne/rote Vorschau** genau dort, wo das Gerät auf der Werkbank liegen wird; belegte Werkbank wird auch über das daraufliegende Gerät erkannt und nennt den Grund.
- Prompts: „E | PC aufnehmen“, „E | PC auf der Werkbank abstellen“, „E | PC bearbeiten“, „R | PC aufnehmen“, „Nichts mehr zu tun – [R] nimmt es von der Werkbank“.
- Inventar-Kontextmenü **„In die Hand nehmen“** (Server prüft freie Hände und Gewicht; schlägt etwas fehl, kommt das Teil zurück ins Inventar). [R] „In die Hand nehmen“ auch für kleine eigene Geräte am Boden.
- Große Geräte werden mittig und tiefer vor dem Körper getragen.
- Der **Kunden-PC darf geöffnet** und Teile dürfen getauscht werden (das Gehäuse selbst nie). Defekte Teile kommen „Defekt“ heraus, das neue Teil wird eingebaut, danach Funktionstest. Geräte ohne Teile-Layout (Radio usw.) weiter über [Q].
- Auftrags-ID und Gerät bleiben über Speichern/Laden erhalten (auf der Werkbank und beim Tragen; Getragenes wird beim Laden dort abgestellt, wo man es hätte fallen lassen).

### PC-System (Phase 1)
- **RAM-Riegel einzeln** (je zwei Halteklammern). Der zweite Riegel ist Teil 12 (angehängt); Spielstand-Version 20 migriert alte PCs (Zustand, Variante und „ausgebaut“ des alten RAM-Teils gelten für beide Riegel).
- **Wärmeleitpaste beim Kühler-Einbau:** eigener Schritt mit der Spritze vor dem Einsetzen, verbraucht 1 Wärmeleitpaste; fehlt sie, sagt der Einbau „Fehlt: Wärmeleitpaste“.
- **Kamera ohne Clipping:** blickt man auf etwas im Gehäuse, bleibt die Kamera steil genug, um durch die offene Seite zu schauen; sonst bleibt sie außerhalb der Gerätebox. Wände, Regale, Kaffeemaschine: die Kamera hält davor an.
- **Nur das Teil unter dem Mauszeiger** bekommt ein Schild („Grafikkarte – klicken“); gesperrte Teile sagen „Prozessor – erst: CPU-Kühler“.
- **PC von Grund auf bauen:** Ersatzteil-Express führt ein **leeres PC-Gehäuse** und die Grundteile (CPU-Kühler, Prozessor, Festplatte, Mainboard, Netzteil). Das Gehäuse heißt „Alter Computer (leeres Gehäuse)“ und kostet nur das Gehäuse.

### Autos (Phase 3)
Sechs eigene Modelle (`Tools/Assets/build_cars.py`, keine Marken, keine Logos): US-Limousine der 80er (Chrom, Doppelscheinwerfer, Weißwandreifen), Kombi mit Dachreling, Pickup mit offener Ladefläche, alter Werkstatt-Pickup (verblichener Lack, rostige Stoßstangen, Leiterträger), 90er-SUV mit Reserverad, Kompaktwagen. Je Auto: Karosserie aus dem Seitenprofil mit gerundeten Kanten und ausgeschnittenen Radläufen, getönte Glaskabine mit Dach, Säulen, Armaturenbrett, Lenkrad und Sitzen, Reifen mit runden Flanken, Felgen/Radkappen mit Radmuttern, Stoßstangen, Grill, Scheinwerfer/Rückleuchten/Blinker (aus), Spiegel, Türgriffe und -fugen, leere Kennzeichen, Unterboden, Auspuff. Echte Größe (4,0–5,3 m), Räder stehen auf z = 0, drei LODs (100/45/18 %). 22 Autos in L_Garage und L_MainMenu an ihren alten Plätzen getauscht, zufällige Farben (9 Lacke), leicht schräg geparkt.

**Ehrliche Einordnung:** Das sind selbst gebaute Modelle aus Grundformen (deutlich besser als die Kenney-Autos, aber nicht fotorealistisch). Frei nutzbare, realistische Automodelle ohne Konto-Download waren nicht zu finden (Poly Haven hat nur das abgedeckte Auto, das schon steht; Sketchfab/CGTrader/Free3D brauchen eine Anmeldung).

## 3. Offene Punkte
- Fotorealistische Autos: brauchen Modelle von Mike (CC0, .glb/.fbx), dann Import statt `build_cars.py`.
- Speichern beim Tragen legt das Gerät beim Laden ab (nicht wieder in die Hand).
- Ausgebaute heile Teile eines Kunden-PCs gehören wie bei Aufrüst-Aufträgen der Werkstatt; der PC muss vollständig und getestet zurück (sonst lässt er sich nicht abliefern), schlechtere Teile kosten Sterne.
- Kabel sind weiter Teil der Stecker-Schritte (kein eigenes Kabelmanagement).
- Präsentation der Werkstatt (Phase 4) nur über die Arbeitsansicht verbessert; keine neuen Werkbank-Modelle.

## 4. Tests (wirklich gelaufen, 2026-10-24)

| Test | Ergebnis |
|---|---|
| Unit-Tests `WorkshopEmpire.*` | 139/139 grün (neu: `Inventory.CustomerDeviceIntoHands`, `Workshop.Assembly.BuildFromEmptyCase`; angepasst: Paste, zweiter Riegel) |
| **A** Auftrag → Palette → Hand → Werkbank → Grafikkarte tauschen → Kühler mit Paste → Funktionstest → von der Werkbank → abliefern → Geld (`-WEShots=QUB`, echte Fokus-/[E]-Pfade für Tragen und Abstellen) | 3 von 3 Läufen bestanden (250,00 € → 295,98 €, Auftrag geschlossen, Gerät weg) |
| **B** Zerlegen bis aufs Gehäuse und Wiederaufbau im Arbeitsmodus mit Funktionstest (`P18R`, `P175W`, Unit-Tests RebuildFromParts/BuildFromEmptyCase) | bestanden (P18R: „Alter Computer“ getestet, nur CD-Laufwerk fehlt) |
| **C** Speichern/Laden mit PC auf der Werkbank und beim Tragen (`QUA`) | bestanden: Auftrag zeigt weiter auf dasselbe Gerät, nichts verloren |
| Alte Spielstände QAOld1–5 (`P21L`) | laden alle (Migration der RAM-Riegel) |
| Inventar-Fallback, grüne/rote Vorschau, belegte Werkbank (`QUA`) | bestanden |
| **D** Autos aus 6 Blickwinkeln im Level (`QUD`), Einzelansichten (`QUC`) | 22 Autos, alle stehen auf z = 0; Straße mit Autos 78–90 fps (Editor, 1280×720) |
| Regression P5–P21G, P21W (echte Mausklicks), Netzwerkpaare A–X, 4 Spieler, Sitzungen, UI 4 Auflösungen, Menü | alle grün (0 Fehler) |

Gefunden und behoben während der Regression: Ein heiler CPU-Kühler aus einem defekten Kunden-PC kam „Beschädigt“ heraus und ließ den Funktionstest scheitern.
## 5. Leistung der Straße (gepackt, 1920×1080, Hoch)

Messung QUD (Blick die Straße entlang, je 1 s, während auf demselben Rechner zwei andere Unreal-Projekte liefen): mit Autos 34–43 ms GPU, ohne Autos 32–34 ms. Die Autos kosten also etwa 1,5–8 ms; der Großteil kommt von der Straßenszene selbst (Lichter, Schatten, Gebäude) und war vorher nicht gemessen. Offener Punkt: Straßenansicht ohne andere Last nachmessen und gezielt optimieren.
