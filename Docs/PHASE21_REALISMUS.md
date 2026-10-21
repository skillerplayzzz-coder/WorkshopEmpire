# Phase 21 – Realismus, Spieltiefe, Animationen und Spielgefühl

Mikes Auftrag vom 2026-10-20 (in der Nachricht „Phase 18“ genannt; Phasen 18–20 sind schon fertig, deshalb hier Phase 21). Dieses Dokument wird nach jedem Arbeitsschritt fortgeschrieben: was umgesetzt, was getestet, was offen ist.

## Schritt 1 – Analyse des Bestands (2026-10-20)

### Was es schon gibt
| Bereich | Stand | Dateien |
|---|---|---|
| Arbeitsansicht an der Werkbank | Kamera kreist ums Gerät (rechte Maustaste), Mausrad zoomt, Klick auf ein Teil holt es heran; Werkzeugleiste 1–9; Maustaste auf Schraube/Clip/Stecker/Deckel/Teil **halten**, bis der Schritt fertig ist; Reihenfolge wie im echten Gerät (Schrauben vor dem Deckel) | `UI/WEWorkViewWidget`, `Workshop/WEWorkPlan`, `Workshop/WEWorkRig` |
| PC mit echtem Teile-Layout | Gehäuse, Teile, ihre Schrauben, Stecker und Klammern als echte Modelle; Schrauben drehen heraus und fallen in eine Ablage, Teile gleiten entlang ihrer Einbaurichtung heraus; Wiedereinbau in umgekehrter Reihenfolge, Funktionstest | `Workshop/WEAssemblyView`, `WEAssemblyDefinition`, `ASM_OldComputer` |
| Andere Geräte (Monitor, Radio, TV, …) | nur generische Ersatzteile aus Würfeln/Zylindern („Rig“): Schraube = Zylinder, Deckel = Platte, Teil = verkleinertes Modell; erledigte Teile fliegen auf eine gedachte Ablage | `WEWorkRig.cpp` |
| Reparatur (Server) | datengetrieben: Diagnose (Multimeter) → je Fehler ausbauen/einbauen → Funktionstest → optional Aufbereiten; beschädigte Ersatzteile können beim Test versagen | `WERepairDefinition`, `WEWorkbench` |
| Zustände | Neu, Sehr gut, Gebraucht, Beschädigt, Defekt, Schrott + Qualität 0–100; **optisch kaum unterschieden** | `Items/WEItemTypes.h` |
| Werkzeuge | nur als Icon in der Leiste, **nie sichtbar in der Hand** | – |
| Mitarbeiter | Lagerarbeiter (Transport), Maschinenbediener, Fahrer; laufen über das Navmesh, Gegenstände werden aber nicht sichtbar getragen/eingelegt, Maschinenbediener **stehen** an der Maschine | `Staff/WEEmployee`, `WEStaffComponent` |
| Aufträge, Bewertungen | Sterne 1–5 je Auftrag (Phase 19) mit Bonus, Durchschnitt gespeichert; kein Firmenruf mit Wirkung, nicht abschaltbar | `Economy/WEContractComponent` |
| Neues Spiel | Firmenname, Tutorial an/aus, LAN; **keine Schwierigkeitsgrade** | `UI/WEMenuWidgets`, `Online/WESessionSubsystem` |
| Lieferungen | Waren erscheinen sofort auf der Lieferpalette | `World/WEDeliveryZone` |
| Atmosphäre | Radio, Kaffee, Katze, Wetter, Tag/Nacht (Phase 20) | `World/WEAtmosphere`, `WEDayNightComponent` |

### Was fehlt (gegen den Auftrag)
1. Werkzeug in der Hand, Schraube haftet magnetisch am Schraubendreher und landet sichtbar in einer echten Schraubenschale.
2. Bei Nicht-PC-Geräten fühlt sich das Zerlegen wie „Bauklötze“ an (Würfel als Schrauben, Platten als Deckel).
3. Kein Auspacken von Ersatzteilen, keine Schutzfolie, Einrast-/Schraubgeräusche nur allgemein.
4. Zustände nicht sichtbar (Kratzer, Schmutz, Vergilbung), Diagnose nur ein Messschritt ohne eigenes Herausfinden.
5. Keine Reinigung mit Druckluft, keine Wärmeleitpaste, kein sichtbarer Erfolgsmoment (Lüfter drehen, Licht an, Bild an).
6. Mitarbeiter wirken statisch.
7. Keine Werkzeugwand, kein Paketbote, keine Schwierigkeitsgrade, Bewertungen nicht abschaltbar und ohne Wirkung auf den Firmenruf.

### Reihenfolge der Umsetzung
Wie im Auftrag (Punkte 2–11). Grundsatz: bestehende Systeme erweitern statt ersetzen; der Server bleibt die einzige Wahrheit für Teile und Inventar (ein Teil ist nie gleichzeitig im Gerät und im Inventar), alle Animationen sind rein lokal und folgen dem replizierten Zustand.

## Schritt 2 – Reparaturkamera und Interaktion (2026-10-20)

**Umgesetzt** (`UI/WEWorkViewWidget`):
- Mausrad zoomt stufenlos in kleinen Schritten **zum Mauszeiger hin** (Punkt auf dem Gerät unter dem Zeiger, sonst die Ebene durch das Gerät); weit herausgezoomt gleitet der Blick zurück zur Gerätemitte. Nahe genug für Schraubenköpfe (bis 12 % der Startentfernung, nie ins Gerät).
- **Mittlere Maustaste** verschiebt den Blick über das Gerät (begrenzt auf das Gerät).
- **Doppelklick** auf eine Schraube/einen Stecker oder **[F]**: Nahansicht genau dort; Doppelklick daneben bzw. [F] ohne Ziel: zurück zum ganzen Gerät.
- Drehen, Verschieben und Zoomen gleiten weich (eigene Zielwerte, die Kamera folgt gedämpft).
- Markierungen: nur was jetzt geht, ist groß und hell; spätere Schritte sind kleine, blasse Punkte. Tastenleiste ergänzt (Mitte: schieben, F: Nahansicht).

**Getestet:** `-WEShots=P21`: Zoom 1,00 → 0,60 zum Zeiger, Nahansicht 0,16 auf die erste Schraube, zurück 0,44; Bilder `P21_01`–`04`.

## Schritt 3 – Werkzeuge in der Hand, sichtbares Zerlegen, Schraubenschale, Verpackung (2026-10-20)

**Umgesetzt:**
- **Werkzeuge als echte Modelle** (`Tools/Assets/build_hand_tools.py`: Schraubendreher, Akkuschrauber, Zange, Seitenschneider, Hammer, Multimeter-Messspitze, Lötkolben, Winkelschleifer, dazu Druckluftspray, Wärmeleitpasten-Spritze, Pinsel für Schritt 5). `AWEToolHand` legt alle Werkzeuge des Spielers in einer Reihe auf die Werkbank; das gewählte wird hochgenommen (Geräusch), folgt dem Zeiger, sitzt beim Arbeiten mit der Spitze auf der Schraube und dreht sich mit ihr (drei Umdrehungen; der Akkuschrauber schnell, mit Motorgeräusch); Zange und Seitenschneider greifen, die Messspitze tippt auf den Messpunkt, der Lötkolben raucht, der Hammer klopft. Ein anderes Werkzeug legt das alte zurück. Der Griff neigt sich vom Betrachter weg, damit das Werkzeug die Schraube nicht verdeckt.
- **Magnetische Schrauben und Schraubenschale:** Jede Werkbank hat eine Schraubenschale (`SM_Gen_ScrewBowl`). Eine gelöste Schraube kommt aus dem Loch, hängt an der Spitze, wird zur Schale getragen und fällt hinein (Klimpern) – beim PC und bei allen anderen Geräten. Die Schrauben bleiben dort liegen; beim Wiedereinbau holt der Schraubendreher sie wieder heraus.
- **Ersatzteile aus der Verpackung:** Beim Einsetzen eines Ersatzteils steht ein offener Karton neben dem Gerät (Klappen öffnen sich, Geräusch); das Teil wird herausgehoben, ein **neues** Teil verliert erst seine **Schutzfolie** (sie wird abgezogen, Geräusch), dann wird es eingesetzt und rastet ein (Einrastgeräusch).
- Neue Geräusche (CC0, Kenney „RPG Audio“): Schrauben in der Schale, Kartonklappen, Folie, Einrasten, Werkzeug aufnehmen/ablegen.

**Getestet:** `-WEShots=P21`: 22 Handgriffe am PC, Schraubendreher in der Hand auf der Schraube (`holding 1`, `active 1`), Schraube am Magnet unterwegs (`carrying 1`), 3 Schrauben in der Schale; `-WEShots=P21B`: Kofferradio komplett von Hand repariert (Messen mit der Messspitze, Karton, Folie, Einsetzen, Funktionstest; danach „Gebraucht“, nicht mehr defekt). Bilder `P21_05`–`08`, `P21B_10`–`19`.

**Offen / Grenzen:** Beim PC-Wiedereinbau (Teile aus dem eigenen Lager) gibt es keinen Karton, nur bei Ersatzteilen in der Reparatur. Werkzeuge sind eigene, einfache Modelle (keine Fremd-Assets mit besserer Qualität gefunden, die frei und passend wären).

## Schritt 4 – Sichtbarer Zustand und Diagnose (2026-10-21)

**Umgesetzt:**
- **Sichtbarer Verschleiß** (`Items/WEWearVisuals`, Material `M_WEWear`): Jedes Gerät bekommt eine Überlagerung aus Schmutz, Kratzern, Vergilbung und (manchmal) sichtbarem Schaden. Die Werte folgen deterministisch aus Zustand, Qualität und Instanz-ID (`FWEItemWear`), sehen also nach dem Laden gleich aus. Neuwertig = sauber; Gebraucht = leichte Spuren; Verkratzt, Stark verschmutzt, Vergilbt je nach Gerät; Beschädigt/Defekt mit Schaden – aber **nicht jeder Defekt ist sichtbar**. Untersuchen zeigt „Optik: …“.
- **Diagnose in Schritten:** Gerät einschalten und beobachten (die Fehlerbilder stehen pro Fehler in den Daten, z. B. „kein Bild, Lüfter läuft“), danach jedes verdächtige Bauteil einzeln messen („defekt / in Ordnung“). Die Befunde stehen in der Schrittliste. Die Fehler werden beim Start der Diagnose festgelegt.

**Getestet:** `-WEShots=P21C` (Reihe Neuwertig → Defekt, Texte je Zustand), `P21B` (Einschalten, Messen, Befunde). Unit-Tests Reparatur 7/7.

**Offen:** Diagnoseprogramme am PC-Bildschirm gibt es nur als Messschritt, nicht als eigene Bildschirmanwendung.

## Schritt 5 – Reinigung, Wärmeleitpaste, Test (2026-10-21)

**Umgesetzt:**
- Neues Werkzeug **Druckluftspray** (Werkzeugladen, 6 €) und Verbrauchsmaterial **Wärmeleitpaste** (Ersatzteilhandel, 3,50 €).
- Nach der Reparatur: **Staub ausblasen** (vier Ecken, Staubwolken, Zischen; die Staubschicht verschwindet), bei PC und Konsole **alte Paste abwischen und neue auftragen** (Pinsel, Spritze, wachsender Klecks). Beides hebt die Qualität um je 5 (max. 95) und wird im Gerät gespeichert (`Cleaned`, `PasteRenewed`, keine neue Savegame-Version nötig).
- **Erfolgsmoment** beim Funktionstest: Licht geht an, Monitor/Fernseher zeigen ein Bild, das Radio spielt kurz, Lüfter/Motor laufen an – ruhig, ohne Effektfeuerwerk.

**Getestet:** Unit-Test `WorkshopEmpire.Repair.MaintenanceCleanAndPaste`; `P21B` Bilder Ausblasen, Abwischen, Paste, Erfolg.

## Schritt 6 – Mitarbeiter mit sichtbarer Arbeit und Pausen (2026-10-21)

**Umgesetzt:**
- **Körperhaltungen** (`EWEWorkPose`, repliziert): Greifen ins Regal/in die Maschine, Tragen (das echte Modell des Gegenstands, Stapel als Karton, fliegt vom Regal in die Hände und von den Händen in die Maschine), Bedienen an der Maschine. Die bestehende Automatisierung bleibt unverändert.
- **Pausen** alle 4–7 Minuten Arbeitszeit, nur mit freien Händen: zur Kaffeemaschine gehen, Tasse trinken; ist ein Kollege in Pause in der Nähe, wird geplaudert (Gesten), sonst setzt man sich auf den nächsten Hocker/Stuhl (Sitzhöhe passt sich dem Möbel an). Danach zurück an die Arbeit. Kein Weg zum Sitz → keine Pause im Leeren, sondern zurück an die Arbeit.
- **Wegfindung verbessert:** Für Dinge auf Möbeln (Kaffeemaschine auf dem Tisch) sucht der Mitarbeiter einen erreichbaren Platz im Kreis darum; Plätze, die ein Kollege schon belegt, kommen zuletzt. Wer festhängt, wird nicht mehr in den Boden versetzt.

**Getestet:** `-WEShots=P21E -WEStaffBreaks`: Tragen, Einlegen, Bedienen, Kaffee an der Maschine, Plausch, Sitzen auf dem Hocker; danach wieder Transport/Bedienen (Log `P21 after break`). Automatisierte Testläufe schalten Pausen ab, damit Regressionen stabil bleiben.

**Offen:** Einfache Reparaturen durch qualifizierte Mitarbeiter (im Auftrag als „später“ markiert) – noch nicht umgesetzt.

## Schritt 7 – Werkzeugwand (2026-10-21)

**Umgesetzt** (`World/AWEToolWall`, Modelle `SM_Gen_Pegboard`, `SM_Gen_PegHook`):
- Die Werkzeugwand ist jetzt eine Lochwand mit einem Haken pro Lagerplatz. **Jedes Werkzeug im Lager hängt sichtbar an seinem Haken** (dieselben Modelle wie in der Reparaturansicht). Leere Haken zeigen den **hellen Umriss** des Werkzeugs, das dort hing (Schattenbrett).
- **[R] Werkzeug aufhängen**: alle Werkzeuge aus dem Inventar an freie Haken. **[Q] „<Werkzeug> nehmen“**: das Werkzeug, auf das man schaut, kommt ins Inventar. [E] öffnet das Lager wie bisher. Geräusch beim Nehmen und Aufhängen.
- **Griffbereit:** Werkzeuge an einer Werkzeugwand im Umkreis von 6 m zählen an der Werkbank wie eigene (Reparaturschritte, Werkzeugleiste der Reparaturansicht).
- Inhalt wird wie jedes Möbellager gespeichert; mehr Lagerplätze durch Ausbau ergeben mehr Haken.

**Getestet:** `-WEShots=P21F`: 7 Werkzeuge aufgehängt (7 hängen), Schraubendreher-Tempo 2,5 direkt von der Wand, Blick auf Haken 0 bietet „Schraubendreher nehmen“, zwei genommen (5 hängen, 2 im Inventar, Umrisse sichtbar), 6 m entfernt zählt der Lötkolben an der Wand nicht mehr (0), das genommene Multimeter schon (1). Bilder `P21F_40`, `40b`, `41`.

**Offen:** Ein Werkzeug, das an der Wand hängt und an der Werkbank benutzt wird, bleibt währenddessen an der Wand sichtbar.

## Schritt 8 – Paketbote (2026-10-21)

**Umgesetzt** (`Economy/UWECourierComponent`, `AWECourier`, Item „Paket“, Modell `SM_Gen_Parcel`):
- Mit der Regel „Paketbote“ (bei jedem neuen Spiel an) kommen Käufe bei den Versandhändlern (Werkzeug-Versand, Ersatzteil-Express, Gebrauchtbörse, Schrotthandel) und aus den Kleinanzeigen **nicht mehr sofort**, sondern als **Paket**. Eine Bestellung = ein Paket, darin genau das Bestellte.
- **Nicht zu häufig:** Der Bote kommt frühestens 75 s nach der Bestellung (Wirtschaft „Entspannt“: schneller) und höchstens alle 2½ Minuten; was bis dahin fällig ist, bringt er auf einmal (bis zu 4 Pakete).
- Der Bote läuft von der Straße zum Lieferbereich, trägt die Pakete als Stapel vor der Brust, stellt sie einzeln auf die Lieferpalette und geht wieder. Meldung „Der Paketbote kommt …“ und „… im Lieferbereich abgestellt“.
- **Pakete aufheben und mit [Q] öffnen:** Der Inhalt liegt danach daneben, das Paket ist weg. Schwerfracht über 31,5 kg (z. B. Großmotoren, Paletten) kommt weiter wie bisher direkt auf die Palette.
- Bestellungen unterwegs und Paketinhalte werden **gespeichert** (Spielstand Version 19); im Auftragsbildschirm steht „Paketbote: N Bestellungen unterwegs, nächster Besuch in etwa … Sekunden“.

**Getestet:** Unit-Test `WorkshopEmpire.Rules.CourierParcels` (Versandregel, Gewichtsgrenze, Speichern/Laden unterwegs, Paket mit genau 2 Schraubendrehern, Öffnen); `-WEShots=P21G`: 3 Bestellungen, Speichern/Laden (3 unterwegs), Bote läuft von der Straße zur Palette, stellt 3 Pakete ab, geht; Speichern/Laden mit liegenden Paketen (3 von 3 Inhalten bekannt); Paket geöffnet → Arbeitsspeicher liegt da. Bilder `P21G_50`–`53`.

**Offen:** Ist das Garagentor zu, geht der Bote nach kurzem Hängen trotzdem bis zur Palette (wie Laufkundschaft).

## Schritt 9 – Schwierigkeitsgrad beim neuen Spiel (2026-10-21)

**Umgesetzt** (`Framework/UWEGameRulesComponent`, Menü „Neues Spiel“):
- Voreinstellungen **Entspannt**, **Realistisch** (Standard), **Experte** und jede Option einzeln (dann „Individuell“): Diagnose erforderlich, Reparaturfehler, Kundenbewertungen, Wirtschaft (Entspannt/Normal/Schwer), Reparaturkomplexität (Einfach/Normal/Detailliert).
- Wirkung im Spiel:
  - *Diagnose aus:* Die Fehler sind bekannt, sobald das Gerät auf der Werkbank liegt – kein „Fehler suchen“.
  - *Reparaturfehler an:* Beim Aus- und Einbauen rutscht der Schraubendreher ab bzw. ein Teil rastet nicht ein (Realistisch 7 %, Experte 12 %, mit besserem Werkzeug seltener). Der Schritt muss wiederholt werden; das Ersatzteil geht nicht verloren.
  - *Wirtschaft:* Einkauf 85 % / 100 % / 115 %, Verkauf 110 % / 100 % / 90 %.
  - *Komplexität:* Arbeitsschritte 70 % / 100 % / 130 % der Zeit; „Einfach“ nie zwei Fehler, „Detailliert“ öfter zwei (auch bei nur beschädigten Geräten).
- Die Regeln stehen im Spielstand. **Alte Spielstände** und Testwelten behalten das Spiel wie bisher (keine Fehler, kein Paketbote).

**Getestet:** Unit-Tests `Rules.PresetsAndOptions`, `Rules.EconomyPrices` (±15 %), `Repair.DifficultyRules` (Entspannt: sofort diagnostiziert, ein Fehler, 70 % Zeit; Experte: Diagnose nötig, Fehlgriffe, trotzdem fertig); `-WEShots=P21M` im Hauptmenü (alle Voreinstellungen, Einzeloption → Individuell, URL-Option hin und zurück). Bilder `P21M_60`–`63`.

## Schritt 10 – Kundenbewertungen und Firmenruf (2026-10-21)

**Umgesetzt** (`UWEContractComponent`):
- Mit der Regel „Kundenbewertungen“ schreibt jeder Kunde zu seinen Sternen einen kurzen Satz; die letzten 12 Bewertungen werden gespeichert. Ein angenommener Auftrag, der verfällt, bringt bei neuen Spielständen eine 1-Stern-Bewertung.
- **Firmenruf** (ab 3 Bewertungen, Schnitt der Sterne): hervorragend / gut / ordentlich / durchwachsen / schlecht. Guter Ruf: ein Auftrag mehr am Brett und bis +10 % Lohn für neue Aufträge; schlechter Ruf: einer weniger und bis −15 %.
- Auftragsbildschirm: Firmenruf, Schnitt, Anzahl und die drei neuesten Bewertungen.
- **Ganz abschaltbar:** Ohne die Regel gibt es keine Sterne, keinen Sterne-Bonus, keine Bewertungen und keinen Einfluss auf Aufträge.

**Getestet:** Unit-Test `Rules.ReviewsAndReputation` (Ruf 5 → +1 Auftrag und mehr Lohn, 20 schlechte → weniger, nur 12 behalten, Regel aus → kein Einfluss).
