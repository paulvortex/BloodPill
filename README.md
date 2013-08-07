Blood Pill - a set of tools to manipulate with .BIG files. It can be used to view the data stored in pill.big - resources file of Blood Omen.

Features
------
Blood Pill is able to decode all filetypes used in the game into readable/listenable form: speech sound, tile, sprite, tim graphich and more.

Feature list includes:
- Packing/Unpacking of .BIG files for PC and PSX versions of BO
- TIM<->TGA autoconversion during packing and unpacking
- RAW ADPCMWAV/OGG autoconversion during packing and unpacking
- Unpacking sprites (enemies and objects) to separate Targa pictures
- Unpacking tiles (level textures) to Targa pictures
- Unpacking the multiobject files ('merged' files that is used on some entries in BO)
- Decoding of .JAM video to Targa frames
- Decoding VAG files to WAV PCM/OGG sounds
- Decoding levels to Targa images (with optional developer info shown)
- Decoding levels into text-based format used by Blood Omnicide legacy loader (early version, still in development)
- Patching .big files
- Support for Blood Omnicide scripted installation

Screenshots
------
Unpacked sprites and tiles:
![unpacked sprites](/images/sprites.jpg)
![unpacked sprites](/images/sprites2.jpg)
![unpacked tiles](/images/tiles.jpg)
JAM decoded frames:
![JAM frame](/images/jam.jpg)
![JAM frame](/images/jam2.jpg)
![JAM frame](/images/jam3.jpg)
Blood Omen maps converted to TGA:
![Extracted map](/images/map.jpg)
![Extracted map](/images/map2.jpg)
![Extracted map](/images/map3.jpg)

Thanks to
------

- Mean Person for figuring out Blood Omen's VAG internals
- Forest [LordHavoc] Hale for cmdlib and memlib
- Balder and Zench from XentaX community (www.xentax.com) for pill.big specs, advices
- Klarth (stevemonaco@hotmail.com) and Raul Sobon (Cheekyboy@2-hot.com) for TIM specs
- Andrey [Rackot] Grachev for bigfile specs
- Ben Lincoln for tile compression decoder
- MisterGrim for Jam Decoder

License
------
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Usage
------
Run a program without parms to get help info

Known bugs
------
There are no known bugs as of yet since the active tool development is over.
If you found any, let me know.

--------------------------------------------------------------------------------
 Version History + Changelog
--------------------------------------------------------------------------------

1.04 (Blood Omnicide internal release)
------
- Added early version of text-based map format exporter.
  Used by Blood Omnicde legacy loader.
- New key -f tga8_32 for -extract action enables exporting images with 32-bit
  palette (palette with alpha-channel), used by tiles.
- Fixed a crash in -extract action.
- Tile and TIM images extraction now supports -colorscale and -colorsub keys
  which scales rgb color, then subtracts a given 0-255 value from it.
- Renamed -colormapscale/-colormapsub into -colorscale/-colorsub for sprites
  extraction.
- Tile extraction now supports -scale2x and -scale4x magnification filter


1.02 (Blood Omnicide internal release)
------
- New key -mapsaveid for -unpack/-mapconvert stages, -s key for -extract action
  Shows entity/object save numbers when extracting map to Targa image.


1.01 (bugfix release)
------
- Fixed a crash when unpacking PS1 pill.big
- VAG speech are sorted to dirs defined in klist.txt just as RAW ADPCM


1.0 (final release)
------
- Map files can be extracted to Targa images (shows base map layout, enemies
  items, speech marks, objects, doors, optionally solid zones, triggers,
  switchable tiles at state on/off etc.
- Cleaned up sprite extraction code


0.9
------
- New -sp prefix shich prints solid pacifier (used by installer)
- New action -bigfile -patch <patchfile> -outfile <new_bigfile.big>
  Patch file are text file with each line definina a single action :
  RAW/DEL/WAV2ADPCM <pill_big_file_hash_or_name> <path_to_a_new_file_contents>
  or <filename> (RAW replace, where filename should match some of original
  blood omen filenames or #hash)
- JAM->TGA converter (-jam <outdir> action), thanks MisterGrim
- Fixed wrong name unhashing for some file which was producing override
  of 640x480 .htm fiels by 320x240 .tim.
- Fixed a rare bug with wrong extension for raw output files


0.8f
------
- New prefix -c: (compact) dont print program caption
- New prefix -f: (function) dont print anything except for errors
- New prefix -cd: change to selected dir before start
- New prefix -ew: (errorwait) wait for key press on error
- Fixed crash when launched with wrong parms
- Fix mkdir when launched with full paths as parms
- Fix program return codes


0.8e
------
- use static MSVC runtimes


0.8c
------
- Fixed export of multi-chunk files (vortout.all and so on)
- Fixed all names of ADPCM speech files to be in right dirs
- New -script action for install script (used by Blood Omnicide installation)


0.8b
------
- Code cleanups
- Optimized -pack action for speed
- file hashes are automatically converted to real file names (thanks
  Ben Lincoln and Andrey [Rackot] Crachev). By default internal hash/filename
  table are used, can be overriden by BO1.csv.
- Tiles unpacking (thanks Ben Lincoln for LZW unpacking)
- Support for PS1 pill.big unpacking (but not packing since there are
  no WAV->VAG/FAG converter)
- Extract action can handle real filenames, old hash filenames should be
  prefixed by # (example: #AD07E8F1)
- When unpacking, original files are stored in original/ folder.
- Null files (with empty contents) are not unpacked anymore
- Unpack action not unpacks fiels with real filenames (use -hashasnames to
  revenr to old behavior)
- Sample scripts improved
- Added filetypes.txt which brings together knowledge about all Blood Omen
  game file types.
- Better support fo PS1 pill.big extraction


0.6a
------
- New -extract action which extracts a given file from pill.big
- Fix a frequent "cannot open file for writing" error
- Removed -vagconvert,-pcm,-ogg,-rawconvert,-noalign keys from -unpack action.
- New -adpcm2wav, -adpcm2pcm, -adpcm2ogg, -raw2tga, -rawnoalign as a replacement.
- Removed -dstdir option from -unpack action. Now destination dir are optional
  -unpack parms (example: -unpack newdstdir).
- Removed -srcdir option from -pack. Same as for -unpack.
- Removed -to key from -list action. Same as for -unpack.
- Removed -lowmem from -unpack, now always using low memory usage path.


0.5
------
- Now can unpack sprite frames (SHD, SDR, SHA files) into Targa pictures.
  Note that sprite unpacking creates VERY big amount of files (about 35000).
- Auto-align sprite frame size for easy creation of GIF.
- Documented -raw action (sprite unpacking)
- Fixed some memory leaks


0.4
------
- New -rawconvert key for -unpack action. Unpacks some sprites into Targa files.
- Default folder for each file type (.tim gues to tim/, .dat to unknown and so on)
- Better support for different pill.big versions
- Fixed broken pill.big -pack action ("wrong file type" error)


0.3
------
- Added -vagconvert key for -unpack action. Automatically converts .VAG/.FAG
  files into WAV or Ogg Vorbis (-ogg key) by SoX lib. Quality 
  degrades during conversion.
- Backwards WAV/OGG -> ADPCM (.VAG/.FAG) conversion for -pack action.
- Simplify syntax for klist.txt. New "path" option to set additional output path.
- New -soxpath "path" key to set a different path to SoX program.
- Fixed null files sometimes being identified as TIM.
- Fixed wrong TIM layers unpacking for German version of BO
- Fixed segfail during -pack


0.2
------
- New -tim2tga for TIM->TGA conversion during -unpack.
  Optional -16to24 enables additional 16->24 bit conversion.
- New action -tga2tim [-bpp x] [-ofs XY] [-mask X for converting single
  TGA images to TIM images.
- New key -tim2tga for -unpack action. Converts all TIM images into TGA
  with support of backward converting during -pack.
- New -nc prefix that disables caption prints.
- New -lowmem key for -pack action. Uses low memory usage path.
- Added detection of RIFF WAVE sounds.
- Known files list not includes all speech files.


0.1
------
- Initial release
- bpill -bigfile "pill.big" -list [-to filename]
  list pill.big contents to external file.
- bpill -bigfile "pill.big" -unpack [-dstdir dir]
  unpacks pill.big contents to external folder (defailt is bigfile)
- bpill -bigfile "pill.big" -pack [-srcdir dir]
  pack unpacked contents back to a new bigfile.
- Known-files-list support to detect RAW ADPCM files