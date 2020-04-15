# Fonts
In order to get e.g. German "Umlauts" displayed on the display using the Adafruit GFX library,
you need to supply your own 8bit Font, which actually contains these symbols.

You can use the ones included here by copying them to the GFX Fonts/ folder.

To create one yourself, you can use the fontconvert utility delivered together with the GFX library.

fontconvert is in .pio/libdeps/nodemcuv2/Adafruit GFX Library_ID13/fontconvert

In order to create your own font, you can follow these steps:

- Download free fonts from http://ftp.gnu.org/gnu/freefont/ and copy the fonts e.g. to ~/freefont
- run`./fontconvert ~/freefont/FreeMono.ttf 11 32 252 > ../Fonts/FreeMono11pt8b.h`
- the above command creats an 11pt Mono Font with symbols 32 through 252, which includes Umlauts and some other symbols (such as the degree symbol °)

In your code:
- `#include <Fonts/FreeMono11pt8b.h>`
- `tft.setFont(&FreeMono11pt8b);`