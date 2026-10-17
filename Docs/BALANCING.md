# WORKSHOP EMPIRE – Balancing und Fortschrittskurve (Phase 16)

## Methode: Langzeit-Playtest als Simulation auf echten Spieldaten

60 Spielstunden lassen sich nicht bei jeder Datenänderung von Hand durchspielen. Deshalb gibt es den Automation-Test `WorkshopEmpire.Balance.ProgressionCurve` (`Source/WorkshopEmpireTests/Private/WEBalanceSimTests.cpp`). Er spielt einen aufmerksamen Spieler über 60 Stunden durch, einmal allein und einmal zu zweit, und schreibt das Ergebnis nach `Saved/Balance/ProgressionCurve.md`.

**Was echt ist (aus dem Spiel gemessen):**
- Preise aus den Markt- und Händlerregeln.
- Zerlegeergebnisse von einer echten Werkbank, gemittelt über 24 Durchläufe.
- Reparaturen mit Teilepreisen und Fehlerwahrscheinlichkeiten.
- Maschinenergebnisse aus den Rezepten, mit Strom und Wartung.
- Preise von Werkzeugen, Upgrades, Gebäuden, Maschinen, Löhnen, Touren, Aufträgen und Meilensteinen aus ihren Katalogen.
- Nachschub der Händler (Menge, Chance, Intervall).

**Was angenommen ist (`FAssumptions`):**

| Annahme | Wert |
|---|---|
| Anteil produktiver Zeit (Rest: Laufen, Menüs, Umsehen) | 70 % |
| Ein Gerät kaufen, holen, zur Werkbank tragen, „Alles nehmen“, verkaufen | 45 s, dazu 2 s je Arbeitsschritt |
| Ersatzteil bestellen und holen | 15 s |
| Gerät ohne Lagerarbeiter zur Maschine tragen | 12 s |
| Lagerarbeiter | 12 Einheiten pro Minute (gemessen: rund 14) |
| Kundenaufträge pro Stunde | 4 (Industrieaufträge ab Stufe 2: 1,5) |
| Laden und Entladen je Tour | 1 min |

**Nicht simuliert:**
- Auktionen.
- Retourenpaletten.
- Der Schrotthaufen (gratis, rund 1 € je Teil).
- Glücksfunde. Sammlerstücke zählen nur mit ihrer Chance in Touren.

**Kaufreihenfolge im Test:**
1. Werkzeuge und kleine Upgrades.
2. Schredder, Starkstrom, Sortiermaschine, Presse, Fertigungsautomat.
3. Transporter mit Fahrer und Lagerarbeiter, Gabelstapler.
4. Werkstatt-Erweiterung, Verpackungsanlage, Maschinenbediener, moderner Lieferwagen, zweiter Schredder.
5. Gewerbehalle, Schmelzofen, Großes Materiallager, Lkw, dritte Linie.
6. Fabrikhalle, Edelmetall-Rückgewinnung, weitere Linien.

Gekauft wird erst, wenn danach noch eine Stunde Löhne und zwei Touren bezahlt werden können.

## Vorher: Daten bis Phase 15

Gemessen mit einer frühen Fassung der Simulation, allein:

- Ab Stunde 6 blieb das Einkommen bei etwa 700–950 €/h stehen.
- Große Werkstatt nach etwa 17 h, Gewerbehalle erst nach etwa 51 h.
- Fabrik und Industrieunternehmen wurden in 60 h nicht erreicht.

**Ursache: Nachschub und Wertschöpfung wuchsen nicht mit.**
- Händler liefern alle 10–30 Minuten dieselben paar Geräte.
- Maschinen verdienten an einem PC rund 1,50 €, die Presse an Stahlwinkeln 0,73 €.
- Die Gewerbehalle mit Schmelzofen hätte sich erst nach über 1.000 Ofenläufen bezahlt gemacht.

## Nachher: Phase 16 (v0.16.0)

| Ziel | allein | zu zweit | Designziel (GAME_DESIGN §5) |
|---|---|---|---|
| Alle kleinen Werkstatt-Upgrades | 0,7 h | 0,3 h | ~1 h |
| Erste Maschine (Schredder) | 1,1 h | – | früh |
| Transporter mit Fahrer | 5,6 h | – | Mitte (5–15 h) |
| Stufe 2 Große Werkstatt | **9,9 h** | 6,6 h | 5–15 h |
| Stufe 3 Gewerbehalle | **18,2 h** | 13,1 h | 15–30 h |
| Stufe 4 Fabrik | **32,4 h** | 25,6 h | 30–60 h |
| Stufe 5 Industrieunternehmen (250.000 € Firmenwert) | **44,7 h** | 37,5 h | spät |
| Meilenstein „Halbe Million“ | 59,5 h | ~50 h | Endgame |
| Firmenwert 1 Mio. € („Werkstatt-Imperium“) | ~90 h (hochgerechnet) | – | Langzeitziel |

**Ergebnis pro Stunde (allein, nach Abzug von Einkauf, Löhnen und Strom):**

| Abschnitt | Ergebnis pro Stunde |
|---|---|
| Garage, Handarbeit und erste Maschinen | 600–1.200 € |
| Große Werkstatt | 2.000–3.600 € |
| Gewerbehalle mit Lkw | 5.700–6.200 € |
| Fabrik mit Edelmetall-Rückgewinnung | rund 16.000 € |

Der Test bricht mit einem Fehler ab, wenn eine Datenänderung die Kurve aus diesen Fenstern schiebt:
- Stufe 2 nach 7–14 h, Stufe 3 nach 14–26 h, Stufe 4 nach 26–42 h, Stufe 5 nach 38–60 h.
- Im Koop schneller, aber Stufe 5 nicht vor 25 h.
- Erstes Upgrade in der ersten Stunde.
- Keine Verluststunde.

## Was geändert wurde (Hebel)

1. **Nachschub wächst mit der Firma.** Neue Abholtouren ab einer Unternehmensstufe:

   | Tour | ab Stufe | Inhalt | Preis | Gewinn je Fahrt nach Verarbeitung (mit den Maschinen der jeweiligen Phase) |
   |---|---|---|---|---|
   | Elektroschrott-Container | 2 | 12 PCs, 8 Monitore, 10 Kabelkartons, 2 Motoren | 120 € | rund 420 € |
   | Kabel- und Motorenschrott | 3 | 30 Kabelkartons, 5 Motoren | 160 € | rund 290 € |
   | Großauflösung Bürogebäude | 3 | 25 PCs, 12 Monitore, 10 Kabelkartons, 2 Motoren | 200 € | 800–1.100 € |
   | Fabrikauflösung | 4 | Großmotoren, Motoren, Kabel, PCs, oft ein Servomotor | 350 € | rund 990 € |

2. **Lkw mit Ladekran** (12.000 €, nach der Gewerbehalle): Touren 2,4-mal so schnell wie mit dem alten Transporter, 40 Plätze, 3 t.
3. **Maschinenketten lohnen sich in der Menge.**
   - Die Sortiermaschine gewinnt mehr Kupfer und Aluminium aus Elektronik.
   - Neue Endgame-Maschine **Edelmetall-Rückgewinnung** (9.000 €, Fabrikhalle): 20 Elektronikschrott werden zu Edelmetallkonzentrat (65 € Grundwert) und 2 Kupfergranulat; Keramik-Prozessoren werden zu 2 Konzentrat.
4. **Gebäudepreise:** Transporter 2.000 € (vorher 2.400 €), Werkstatt-Erweiterung 5.000 € (vorher 6.000 €), Fabrikhalle 75.000 € (vorher 90.000 €).
5. **14 neue Aufträge** über das ganze Spiel:
   - Aufbereitete PCs, Netzteile, Elektromotoren.
   - Sammleraufträge für jedes Sammlerstück (×1,7–1,9).
   - Schulausstattung, Kupferdrahtspulen.
   - Aluminiumballen, Liefervertrag Aluminiumbarren.
   - Edelmetallkonzentrat, Liefervertrag Kabelpaletten.
   - Ab Stufe 5: Großprojekt Kupferbarren, Exportauftrag Stahlwinkel-Paletten.
6. **31 Meilensteine** mit Prämien als Bonus von etwa 15–30 Minuten Einkommen der jeweiligen Phase, nicht als Antrieb. Bis Stufe 5 kommen rund 30.000 € zusammen. Danach folgen „Halbe Million“ und „Werkstatt-Imperium“ (1 Mio. € Firmenwert) als Langzeitziele.
7. **Seltene Gegenstände:**
   - Sammlerstücke bringen bei Sammleraufträgen fast das Doppelte ihres Marktwerts. Vorher war ein Kauf beim Händler ein Verlustgeschäft.
   - In den großen Touren tauchen sie mit 3–25 % Chance auf.
   - Der Meilenstein „Sammlervitrine“ belohnt alle vier Sorten.

## Neu ausführen

```
UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests WorkshopEmpire.Balance;Quit" -unattended -NullRHI -nosteam -testexit="Automation Test Queue Empty"
```

Danach `Saved/Balance/ProgressionCurve.md` lesen. Es enthält:
- Kaufzeitpunkte und Ergebnis pro Stunde, allein und zu zweit.
- Einnahmen je Bereich pro Stunde.
- Werte je Einheit für alle Wege (Verkauf, Zerlegen, Reparieren, Maschinen).
- Gewinn je Tour.

Eine Kopie des Laufs vom 2026-10-17 liegt in `Docs/ProgressionCurve_Phase16.md`.
