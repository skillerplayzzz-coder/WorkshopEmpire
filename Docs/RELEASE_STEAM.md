# Release auf Steam – Anleitung (Phase 20)

Alles, was ohne Mikes Steamworks-Konto geht, ist vorbereitet. Was nur Mike machen kann, ist markiert.

## 1. Eigene Steam App ID (nur Mike)

1. Auf https://partner.steamgames.com ein Steamworks-Konto anlegen und die Steam-Direct-Gebühr (100 US-Dollar je Spiel) bezahlen.
2. Eine neue App anlegen. Steam vergibt die **App ID** (z. B. `1234567`) und eine **Depot ID** (meist App ID + 1).
3. Im Projekt eintragen (ein Befehl, ändert alle Stellen):

   ```
   powershell -File Tools\Release\set_steam_appid.ps1 -AppId 1234567 [-DepotId 1234568]
   ```

   Das Skript setzt in `Config/DefaultEngine.ini` unter `[OnlineSubsystemSteam]` den Wert `SteamDevAppId` (die Engine liest ihn in jeder Build-Art) und `bRelaunchInSteam=true` (wird das Spiel ohne Steam gestartet, startet es sich über Steam neu). Außerdem schreibt es die Upload-Skripte `Tools/Release/steam/app_build.vdf` und `depot_build.vdf`. Bis dahin steht dort `480` (Valves Test-App „Spacewar“), damit Koop über Steam schon jetzt testbar ist.

## 2. Shipping-Build

```
powershell -File Tools\Release\package_shipping.ps1 [-Name Release]
```

Ergebnis: `F:\Spiele\WORKSHOP EMPIRE\Builds\<Name>\Windows\WorkshopEmpire.exe` (außerhalb des Repositorys). Shipping enthält keine Konsole und keine Testautomatik. Die App ID muss **vor** dem Packen eingetragen sein.

## 3. Speicherstände in der Cloud (Steam Auto-Cloud, nur Mike im Steamworks-Backend)

Das Spiel speichert im Shipping-Build unter `%LOCALAPPDATA%\WorkshopEmpire\Saved\` (geprüft mit `Builds/P20_Shipping`). Auto-Cloud braucht keinen Code, nur diese Einträge unter *App Admin → Cloud → Auto-Cloud*:

| Root | Unterverzeichnis | Muster | Betriebssystem | Zweck |
|---|---|---|---|---|
| `WinAppDataLocal` | `WorkshopEmpire/Saved/SaveGames/WorkshopEmpire` | `*.wesave` | Windows | Firmen-Spielstände (auch `Autosave`) |
| `WinAppDataLocal` | `WorkshopEmpire/Saved/SaveGames/WorkshopEmpire` | `*.wesave.bak` | Windows | Sicherungskopie des letzten Stands |
| `WinAppDataLocal` | `WorkshopEmpire/Saved/SaveGames` | `WEProfile.sav` | Windows | Spielerprofil (Name, Aussehen) |

Empfohlen: Byte-Kontingent 100 MB, Anzahl Dateien 200. Danach „Save“ und „Publish“. Einstellungen und Tastenbelegung (`GameUserSettings.ini`) bleiben bewusst lokal, weil sie vom jeweiligen Rechner abhängen.

**Erst mit der echten App ID testbar:** Cloud-Sync zwischen zwei Rechnern. Mit 480 synchronisiert Steam nichts für dieses Spiel.

## 4. Hochladen (nur Mike, mit eigenem Steamworks-Login)

1. Steamworks SDK herunterladen, `tools/ContentBuilder/builder/steamcmd.exe` verwenden.
2. `steamcmd +login <Benutzer> +run_app_build "<Pfad>\Tools\Release\steam\app_build.vdf" +quit`
3. Im Backend den Build auf einen Branch setzen (zuerst `beta`, nach dem Test `default`).

## 5. Trailer

Der Trailer entsteht aus echten Spielszenen:

```
UnrealEditor.exe WorkshopEmpire.uproject /Game/WorkshopEmpire/Maps/L_Garage -game -windowed -ResX=1920 -ResY=1080 -nosteam -WEShots=P20T -UseFixedTimeStep -FPS=30
powershell -File Tools\Release\make_trailer.ps1
```

Das Spiel fährt mit einer Kamera durch 9 Szenen (Werkstatt am Morgen, Werkbank mit Radio und Kaffeemaschine, die Katze, Mitarbeiter, Logo-Editor, Transporter, Regen, Gewitter bei Nacht, Firmenschild am Abend) und schreibt jedes Bild nach `Saved/Trailer`. Das Skript macht daraus mit ffmpeg ein 52-Sekunden-Video (1920×1080, H.264) mit Titel, Untertiteln, Abspann und Musik (Holizna „Make Money“, CC0), Regen und Donner (CC0): `F:\Spiele\WORKSHOP EMPIRE\Builds\Trailer\WorkshopEmpire_Trailer.mp4`. Steam verlangt für die Shopseite mindestens ein Video; dieses kann direkt hochgeladen werden.

## 6. Noch offen für die Veröffentlichung (Entscheidungen von Mike)

- Shopseite: Beschreibung, Screenshots (mindestens 5), Kapselbilder in den Steam-Formaten, Altersfreigabe-Fragebogen, Preis.
- Rechtliches: Impressum/Anbieterangaben im Steamworks-Konto, Steuerformulare.
- Vor der Veröffentlichung die Lizenzen aller Fremd-Assets in `Docs/ASSET_SOURCES.md` noch einmal durchsehen (Quelle und Lizenz stehen dort je Paket).
