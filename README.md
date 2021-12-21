# Payload Guest

## Synopsis

Payload Guest reads payloads from `/data/payloads/` (From the PS4's internal hard drive) and/or `/mnt/usb*/payloads/` (From a USB devices `/payloads/` directory). If you have a `meta.json` file in one of these directories it will parse that file rather than trying to scan that particular directory's files. If there is no `meta.json` file the application will scan the folder and add any `.bin` files found to the menu, it will look for a `.png` file with the same name to display. Pressing square will refresh the list (For if you inserted/removed a USB device).

## Why?

Because the success rate for re-exploiting will be lower than just running the code "natively." This should completely replace exploit hosts for all but one payload, no need for a maze of buttons or questionable "stability tweaks." This also puts you in more control of what you run on your system. You'll still depend on a different solution for HEN/Mira/GoldHEN but... make good choices.

## Example `meta.json`

Please note the open and closing square brackets (`[ ]`).

```json
[
  {
    "name": "Enable Browser",
    "filename": "enable-browser.bin",
    "icon": "enable-browser.png"
  },
  {
    "name": "Disable ASLR",
    "filename": "different-name.bin",
    "icon": "no-match.png"
  }
]
```

## License

Please take notice of the [LICENSE](https://github.com/Al-Azif/ps4-payload-guest/blob/main/LICENSE). It's GPL-3, meaning if you modify this you MUST provide your source along with other requirements. You can find more info [here](https://tldrlegal.com/license/gnu-general-public-license-v3-(gpl-3)).

## TODO

This is a to do list of know issues/planned features. If all of these manage to get done it's essentially 100% complete and 100% overengineeered for what it was supposed to be.

- [ ] Running `make` twice without `make clean` between causes a `error: duplicate symbol: main`
- [ ] Run payload in separate process vs just a separate thread (To avoid bad payloads crashing the app)
  - Check PayloadsView.cpp:L311 for switching how the payload is currently being loaded
  - [ ] Is it possible to notifi() on error signal/coredump within fork'd process?
- [ ] Swap to SDL2 for rendering (May fix issues below)
- [ ] Directly use system typefaces, font library, PNG library, etc (May fix issues below)
- [ ] Use `error_notifi.png` for error notifications
- [ ] "Fix" create-gp4 in the SDK so directories aren't static (Then organize assets into subfolders)
- [ ] Get PNG transparency working correctly
- [ ] Arabic is rendered backwards
  - [ ] Arabic likely needs to use harfbuzz to properly render ligatures
- [ ] Payload title centering is a off when value is used from the fallback typeface (Arabic/Thai)
- [ ] Fix newline height is based on font "width" right now, so it can be different than what's expected (Arabic/Thai)
- [ ] Setup proper logging levels/statements (Everything is currently just `LL_Debug`)

- [ ] New options menu (Options button, save to application save file)
  - [ ] Set log level to display in logging
  - [ ] Set language to something other than the system language
  - [ ] Hide payload location on
  - [ ] Set "safety delay" timer value. Determines how many seconds must pass before another payload can be loaded after loading another.
  - [ ] Enable a payload listener on a custom port
  - [ ] Send payload to an IP/port rather than executing directly
  - [ ] Sorting order (Default, alphabetical, most used, etc)
  - [ ] Move `CreditView` into options (Include credits for translators)

- [ ] Make it look better visually (Should be ongoing process)
  - [ ] Stay simple
  - [ ] Work towards looking more and more native without adding unnecessary active elements
  - [ ] Menu Sounds
  - [ ] Add theme support (Distribute as AC PKGs)
