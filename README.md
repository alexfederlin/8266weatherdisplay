# 8266weatherdisplay
A weatherdisplay project based on http://educ8s.tv/art-deco-weather-forecast-display/

I improved it in a couple of ways:

- Created different areas to display the different information elements. This makes it a little easier to adapt the code to different screen sizes. As scaling of the weather bitmaps is not possible, this will only go so far.
- definitions for 80x160 / 128x160 and 240x240 screens are included
- added a description area in which the Weather description and wind direction and speed can be displayed
- fixed the JSON parsing and adapted it to ArduinoJSON 6
- Introduced 8bit Fonts in order to display German "Umlauts" in the weather description (see Fonts directory)
- As openweathermap transmits the weather description in UTF8 encoding, the tft.print function expects 8859-1 encoded data, I also had to add some code to to the conversion
- made the code a bit nicer in that it now uses a struct to store the weather data
- in fact, I made it an array of struct, to store the weather information of the next 24 hours and flip through them
- got rid of the busy waiting in the loop(). I'm looking to add functionality like flipping through the pages using a button or rotary encoder
- included the ezTime library for dealing with timezones more easily
