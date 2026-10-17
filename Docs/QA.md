# WORKSHOP EMPIRE – Qualitätssicherung (Phase 17)

Alle Prüfungen sind wiederholbar. Alle automatischen Läufe laufen mit `-nosteam`.

**Abkürzungen in den Befehlen:**
- `<exe>` = `F:\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe`
- `<cmd>` = `UnrealEditor-Cmd.exe`
- `<uproject>` = Pfad zu `WorkshopEmpire.uproject`

## 1. Automatische Tests

```
<cmd> <uproject> -ExecCmds="Automation RunTests WorkshopEmpire;Quit" -unattended -nopause -NullRHI -NoSound -nosteam -testexit="Automation Test Queue Empty" -ReportExportPath=<Ordner>
```

Neu in Phase 17 (`WEQualityTests.cpp`):

| Test | Prüft |
|---|---|
| `Save.DamagedFilesNeverCrash` | 131 kaputte Varianten eines echten Spielstands: leer, abgeschnitten, Bitfehler, Müll mit gültiger Kennung, angehängter Müll. Alle werden sauber abgelehnt. |
| `Save.RepeatedSaveLoadIsStable` | 10-mal speichern und laden ergibt genau dieselbe Firma. |
| `Save.OlderVersionsLoad` | Spielstände der Versionen 12, 11, 8 und 4 laden. |
| `Save.LargeWorldIsFast` | 2.000 Gegenstände: Speichern unter 1 s, Laden unter 3 s, Datei unter 5 MB. |

## 2. Leistung einer vollen Fabrik

```
<exe> <uproject> /Game/WorkshopEmpire/Maps/L_Garage -game -windowed -ResX=1920 -ResY=1080 -nosteam -WEShots=P17
```

Der Lauf baut eine volle Fabrik auf:
- Alle Gebäude, 28 laufende Maschinen in Halle und Fabrik.
- 20 Mitarbeiter, davon 10 Bediener und 10 Lagerarbeiter.
- 400 lose Gegenstände.

Gemessen wird an vier Stellen. Je Ansicht steht im Log eine Zeile `PERF <Ansicht>` mit fps, Game-Thread, Render-Thread, GPU, Speicher und Actors. Danach speichert und lädt der Lauf die ganze Fabrik und vergleicht alle Zählwerte (`STRESS S4–S6`).

Langlauf: `-WEStressMinutes=30` lässt die Fabrik 30 Minuten laufen und loggt jede Minute fps und Speicher (`soak_NNN`).

## 3. Multiplayer-Stresstest

Dedizierter Server mit `-WEServerMonitor` (loggt alle 10 s `MONITOR …`: Bildzeit, Verkehr, Speicher, Gegenstände, Konto) und 4 Clients mit `-WESmokeRole=Z -WEStressMinutes=N`.

Jeder Client macht zweimal pro Sekunde eine zufällige Aktion:
- aufheben oder ablegen
- kaufen oder verkaufen
- Inventar verschieben oder sortieren
- ablegen aus dem Inventar
- Tor bedienen
- sich versetzen
- etwas erhalten

Dazwischen läuft er herum.

Netzwerkstörungen per Kommandozeile:
- Client 3: `-PktLag=150 -PktLagVariance=30 -PktLoss=3`
- Client 4: `-PktLag=300 -PktLoss=10 -PktOrder=1 -PktDup=2`

Skript: siehe TEST_REPORT (Phase 17). Kopftest-Clients ohne Grafik laufen automatisch mit höchstens 120 fps.

## 4. UI und Eingabe

```
<exe> <uproject> -game -windowed -ResX=<B> -ResY=<H> -nosteam -WEShots=P17UI_<Name>
<exe> <uproject> /Game/WorkshopEmpire/Maps/L_Garage -game -windowed -ResX=<B> -ResY=<H> -nosteam -WEShots=P17UI_<Name>
```

Der erste Lauf zeigt das Hauptmenü, der zweite das Spiel.

**Tasten:** Sie gehen über den echten Weg (Viewport → PlayerController → Enhanced Input) und werden einige Frames gedrückt gehalten:
- W, S mit Shift (Sprint), A, D, Leertaste, Strg
- Tab und I (Inventar), E am Marktplatz, B (Baumodus), Esc (Menü), F5 (Speichern), F (Untersuchen)

**Schließen:** Tab und Esc schließen Fenster über Slate, so wie das offene Fenster die Taste bekommt.

**Log:** Für jede Prüfung steht dort `INPUT …: ok/FAIL`. Für jedes Fenster steht `UI <Fenster>: needs B x H px (DPI scale), viewport … -> fits/TOO BIG`.

**Bilder:** Je Fenster entsteht ein Screenshot. Die Screenshots werden angesehen, weil Text, der innerhalb eines Fensters überläuft, nur im Bild auffällt.

## 5. Bekannte Grenzen der Prüfung

- **Breitbild 3440 × 1440:** Auf dem Entwicklungs-PC nicht prüfbar, weil der Bildschirm kleiner ist und Windows das Fenster auf 1920 × 1080 begrenzt. Die Oberfläche skaliert über die Höhe und ist zentriert.
- **Gamepad:** Nicht vorgesehen und nicht geprüft. Das Spiel ist auf Maus und Tastatur ausgelegt.
- **Steam:** Steam-Mehrspieler über zwei Rechner prüft Mike (siehe Phase 14).
