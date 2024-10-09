<p align="center">
  <img src="https://github.com/TwnKey/YCTT/assets/69110695/0af45f20-7c04-4145-9f74-af612947d4bc" width="500" height=auto>
</p>

# Translation projects for Ys Chronicles / Ys Chronicles的翻译项目

- [Spanish/西班牙语](https://tradusquare.es/proyectos/ys1-chronicles/)

- [French/法语](https://nanami-ys.fr/?page=Home)

- [Ukrainian/乌克兰语](https://www.nexusmods.com/ysiandiichronicles/mods/1)

# YCTT

This repository provides a set of tools to translate the Ys Chronicles games, mostly Ys II at the moment. If you want to translate Ys I, the font converter and ysext are usable but the script editing is different. See [AdolTranslator](https://github.com/Darkmet98/AdolTranslator) by Darkmet for translating Ys I.  

此存储库提供了一套工具来翻译Ys编年史游戏，目前主要是Ys II。如果你想翻译Ys I，字体转换器和ysext可以使用，但脚本编辑是不同的。参见Darkmet的[AdolTranslator](https://github.com/Darkmet98/AdolTranslator)以翻译Ys I。

# How to translate the game in your language / 如何将游戏翻译成你的语言

For Ys II, see the [wiki in english](https://github.com/TwnKey/YCTT/wiki/%5BEN%5D-%E2%80%90-How-to-translate-Ys-II-Chronicles) or [chinese](https://github.com/TwnKey/YCTT/wiki/%5BCN-%7C-%E7%94%B1-ChatGPT-%E7%BF%BB%E8%AF%91%5D-%E5%A6%82%E4%BD%95%E7%BF%BB%E8%AF%91-Ys-II-Chronicles)  

对于Ys II，请参阅[英文维基](https://github.com/TwnKey/YCTT/wiki/%5BEN%5D-%E2%80%90-How-to-translate-Ys-II-Chronicles)或[中文维基](https://github.com/TwnKey/YCTT/wiki/%5BCN-%7C-%E7%94%B1-ChatGPT-%E7%BF%BB%E8%AF%91%5D-%E5%A6%82%E4%BD%95%E7%BF%BB%E8%AF%91-Ys-II-Chronicles)

# ImageConverter

The ImageConverter tool is a new addition to YCTT. It was made to edit the images in the game. However, it is still very experimental and things might not work as expected. The game has two image formats (at least): .256 and .SKI. .256 are simple pictures with no compression (and I believe, no transparency). .SKI files have both transparency and compression.

To extract a .256 file as a .bmp file for editing, run the following command:
ImageConverter.exe <Path to .PAL file to use (COLOR.PAL or COLORF.PAL are both fine)> <Path to the .256 file> -e -p<PaletteID>

To insert it back into the game:
ImageConverter.exe <Path to .PAL file to use (COLOR.PAL or COLORF.PAL are both fine)> <Path to the folder containing the previously extracted .BMP> -i256

To extract a .ski file as .bmp files (two for each frame) for editing, run the following command:
ImageConverter.exe <Path to .PAL file to use (COLOR.PAL or COLORF.PAL are both fine)> <Path to the .SKI file> -e

To insert them back into the game:
ImageConverter.exe <Path to .PAL file to use (COLOR.PAL or COLORF.PAL are both fine)> <Path to the folder containing the previously extracted .BMP> -iski

The .256 files require to specify the palette ID!!! Otherwise results are very likely to have wrong colors. Unfortunately the palette id used for a specific image is specified in the .exe. You have to manually find it by first extracting your .256 file in all the palettes. To do this run the following command:
ImageConverter.exe <Path to .PAL file to use (COLOR.PAL or COLORF.PAL are both fine)> <Path to the .256 file> -e -allp

The -allp option will extract one version of the file in the color of each palettes in the .PAL file. Then you have to find the correct colors by matching the file with the picture you know in the game.

Finally, SKI files are made of two layers, which are two bmp, one normal bmp and one "special" (it's in the filename) BMP. This special BMP can only use three colors, red, green, blue, but your pixels can have 16 levels of transparency. The actual color used in the game will likely not be blue, green or red however, those are just to separate between three colors. The normal bmp should only use the colors that are present when first extracting.

Anyway, for SKI files, edit both the special and the normal bmp following the adequate rules.


