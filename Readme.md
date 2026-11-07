# Groovy Dolphin (updated)

This is Groovy Dolphin moved over to current Dolphin. It sends GameCube and Wii games from your PC to a MiSTer running the [Groovy core](https://github.com/psakhis/Groovy_MiSTer), so you can play them on a real CRT. Video, audio and controllers plugged into the MiSTer all go over the network cable.

The Groovy side of this is psakhis's work, built on Martin Dahl's ([m-dahl](https://github.com/m-dahl/groovy-dolphin)) original Groovy Dolphin. psakhis passed away in 2024 and his Dolphin version was stuck on a 2024 build, so I wanted to keep it going. Huge thanks to both of them.

## Download

**[Latest Windows build](https://github.com/zander2k01/groovy-dolphin/releases/download/latest/groovy-dolphin-windows-x64.zip)**

Unzip it anywhere and run Dolphin.exe. It's portable, so your settings and saves stay in that folder and won't mess with a normal Dolphin install.

## What you need

- A MiSTer with the Groovy core set up: `MiSTer_groovy` in the root of the SD card, the `Groovy_....rbf` file in `_Utility`, and this at the end of `MiSTer.ini`:
  ```
  [Groovy]
  main=MiSTer_groovy
  ```
- A wired gigabit connection between the PC and the MiSTer (direct cable or a gigabit switch).
- In the Groovy core's own menu on the MiSTer, turn **Audio** on (it's off by default). If you want to use a controller plugged into the MiSTer, also set **Server > Joysticks** to Analog.

## Setup

1. Load **Utility > Groovy** on the MiSTer.
2. In Dolphin go to **Settings > Graphics**. In the Groovy MiSTer box, tick **Enable MiSTer Output** and type in your MiSTer's IP.
3. Pick a TV mode (Auto NTSC/PAL is a good start) and start a game.

If the MiSTer isn't ready yet, Dolphin keeps trying on its own and tells you what's going on in its window.

## What's new compared to psakhis's last build

- Based on current Dolphin instead of a 2024 one
- The picture is the same size as on a real GameCube now, so the edges don't get cut off (the overscan problem psakhis mentioned)
- Fixed junk staying on screen with Delta Frames on
- Reconnects by itself if the Groovy core gets started late or restarted
- On-screen messages for things like "can't find the MiSTer" or "audio is off in the Groovy core"
- 240p option per game, already on for some retro collections (Sonic, Mega Man, Midway, Namco Museum)
- PAL games play at the right speed in 240p mode (288p)
- NetPlay works with friends on official Dolphin, as long as they use the version shown in Help > About
- A bunch of smaller fixes (audio crackle, a possible crash with PAL games, the controller code)

## Known issues

I've mostly tested on my own setup (Windows, a Sony Trinitron, Pikmin 2), so other games and TVs might do weird things. If something looks off, tick **Show Sent Frame Window** in the Groovy settings. If that window looks fine but the CRT doesn't, the problem is in the sending part.

---

# Dolphin - A GameCube and Wii Emulator

[Homepage](https://dolphin-emu.org/) | [Project Site](https://github.com/dolphin-emu/dolphin) | [Buildbot](https://dolphin.ci/) | [Forums](https://forums.dolphin-emu.org/) | [Wiki](https://wiki.dolphin-emu.org/) | [GitHub Wiki](https://github.com/dolphin-emu/dolphin/wiki) | [Issue Tracker](https://bugs.dolphin-emu.org/projects/emulator/issues) | [Coding Style](https://github.com/dolphin-emu/dolphin/blob/master/Contributing.md) | [Transifex Page](https://app.transifex.com/dolphinemu/dolphin-emu/dashboard/) | [Analytics](https://mon.dolphin-emu.org/)

Dolphin is an emulator for running GameCube and Wii games on Windows,
Linux, macOS, and recent Android devices. It's licensed under the terms
of the GNU General Public License, version 2 or later (GPLv2+).

Please read the [FAQ](https://dolphin-emu.org/docs/faq/) before using Dolphin.

## System Requirements

### Desktop

* OS
    * Windows (10 1903 or higher).
    * Linux.
    * macOS (11.0 Big Sur or higher).
    * Unix-like systems other than Linux are not officially supported but might work.
* Processor
    * A CPU with SSE2 support.
    * A modern CPU (3 GHz and Dual Core, not older than 2008) is highly recommended.
* Graphics
    * A reasonably modern graphics card (Direct3D 11.1 / OpenGL 3.3).
    * A graphics card that supports Direct3D 11.1 / OpenGL 4.4 is recommended.

### Android

* OS
    * Android (7.0 Nougat or higher).
* Processor
    * A processor with support for 64-bit applications (either ARMv8 or x86-64).
* Graphics
    * A graphics processor that supports OpenGL ES 3.0 or higher. Performance varies heavily with [driver quality](https://dolphin-emu.org/blog/2013/09/26/dolphin-emulator-and-opengl-drivers-hall-fameshame/).
    * A graphics processor that supports standard desktop OpenGL features is recommended for best performance.

Dolphin can only be installed on devices that satisfy the above requirements. Attempting to install on an unsupported device will fail and display an error message.

## Building

You may find building instructions on the appropriate wiki page for your operating system:

* [Windows](https://github.com/dolphin-emu/dolphin/wiki/Building-for-Windows)
* [Linux](https://github.com/dolphin-emu/dolphin/wiki/Building-for-Linux)
* [macOS](https://github.com/dolphin-emu/dolphin/wiki/Building-for-macOS)
* [Android](#android-specific-instructions) <!-- TODO: Create a "Building for Android" wiki page and link it here -->
* [OpenBSD](https://github.com/dolphin-emu/dolphin/wiki/Building-for-OpenBSD) (unsupported)

Before building, make sure to pull all submodules:

```sh
git submodule update --init --recursive
```

### Android-specific instructions

These instructions assume familiarity with Android development. If you do not have an
Android dev environment set up, see [AndroidSetup.md](AndroidSetup.md).

If using Android Studio, import the Gradle project located in `./Source/Android`.

Android apps are compiled using a build system called Gradle. Dolphin's native component,
however, is compiled using CMake. The Gradle script will attempt to run a CMake build
automatically while building the Java code.

## Uninstalling

On Windows, simply remove the extracted directory, unless it was installed with the NSIS installer,
in which case you can uninstall Dolphin like any other Windows application.

Linux users can run `cat install_manifest.txt | xargs -d '\n' rm` as root from the build directory
to uninstall Dolphin from their system.

macOS users can simply delete Dolphin.app to uninstall it.

Additionally, you'll want to remove the global user directory if you don't plan on reinstalling Dolphin.

## Command Line Usage

```
Usage: Dolphin.exe [options]... [FILE]...

Options:
  --version             show program's version number and exit
  -h, --help            show this help message and exit
  -u USER, --user=USER  User folder path
  -m MOVIE, --movie=MOVIE
                        Play a movie file
  -e <file>, --exec=<file>
                        Load the specified file
  -n <16-character ASCII title ID>, --nand_title=<16-character ASCII title ID>
                        Launch a NAND title
  -C <System>.<Section>.<Key>=<Value>, --config=<System>.<Section>.<Key>=<Value>
                        Set a configuration option
  -s <file>, --save_state=<file>
                        Load the initial save state
  -d, --debugger        Show the debugger pane and additional View menu options
  -l, --logger          Open the logger
  -b, --batch           Run Dolphin without the user interface (Requires
                        --exec or --nand-title)
  -c, --confirm         Set Confirm on Stop
  -v VIDEO_BACKEND, --video_backend=VIDEO_BACKEND
                        Specify a video backend
  -a AUDIO_EMULATION, --audio_emulation=AUDIO_EMULATION
                        Choose audio emulation from [HLE|LLE]
```

Available DSP emulation engines are HLE (High Level Emulation) and
LLE (Low Level Emulation). HLE is faster but less accurate whereas
LLE is slower but close to perfect. Note that LLE has two submodes (Interpreter and Recompiler)
but they cannot be selected from the command line.

Available video backends are "D3D" and "D3D12" (they are only available on Windows), "OGL", and "Vulkan".
There's also "Null", which will not render anything, and
"Software Renderer", which uses the CPU for rendering and
is intended for debugging purposes only.

## DolphinTool Usage
```
usage: dolphin-tool COMMAND -h

commands supported: [convert, verify, header, extract]
```

```
Usage: convert [options]... [FILE]...

Options:
  -h, --help            show this help message and exit
  -u USER, --user=USER  User folder path, required for temporary processing
                        files. Will be automatically created if this option is
                        not set.
  -i FILE, --input=FILE
                        Path to disc image FILE.
  -o FILE, --output=FILE
                        Path to the destination FILE.
  -f rvz|iso|gcz|wia, --format=rvz|iso|gcz|wia
                        Container format to use. Default is RVZ.
  -s, --scrub           Scrub junk data as part of conversion.
  -b BLOCK_SIZE, --block_size=BLOCK_SIZE
                        Block size for GCZ/WIA/RVZ formats, as an integer.
                        Suggested value for RVZ: 131072 (128 KiB)
  -c none|zstd|bzip2|lzma|lzma2, --compression=none|zstd|bzip2|lzma|lzma2
                        Compression method to use when converting to WIA/RVZ.
                        Suggested value for RVZ: zstd
  -l COMPRESSION_LEVEL, --compression_level=COMPRESSION_LEVEL
                        Level of compression for the selected method. Ignored
                        if 'none'. Suggested value for zstd: 5
```

```
Usage: verify [options]...

Options:
  -h, --help            show this help message and exit
  -u USER, --user=USER  User folder path, required for temporary processing
                        files. Will be automatically created if this option is
                        not set.
  -i FILE, --input=FILE
                        Path to input file.
  -a crc32|md5|sha1|rchash, --algorithm=crc32|md5|sha1|rchash
                        Optional. Compute and print the digest using the
                        selected algorithm, then exit.
```

```
Usage: header [options]...

Options:
  -h, --help            show this help message and exit
  -i FILE, --input=FILE
                        Path to disc image FILE.
  -b, --block_size      Optional. Print the block size of GCZ/WIA/RVZ formats,
then exit.
  -c, --compression     Optional. Print the compression method of GCZ/WIA/RVZ
                        formats, then exit.
  -l, --compression_level
                        Optional. Print the level of compression for WIA/RVZ
                        formats, then exit.
```

```
Usage: extract [options]...

Options:
  -h, --help            show this help message and exit
  -i FILE, --input=FILE
                        Path to disc image FILE.
  -o FOLDER, --output=FOLDER
                        Path to the destination FOLDER.
  -p PARTITION, --partition=PARTITION
                        Which specific partition you want to extract.
  -s SINGLE, --single=SINGLE
                        Which specific file/directory you want to extract.
  -l, --list            List all files in volume/partition. Will print the
                        directory/file specified with --single if defined.
  -q, --quiet           Mute all messages except for errors.
  -g, --gameonly        Only extracts the DATA partition.
```
