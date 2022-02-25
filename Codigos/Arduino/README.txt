#README

##Instalacion de librerias

Instalar tarjeta para el ESP01 (basado en ESP8266):

http://arduino.esp8266.com/stable/package_esp8266com_index.json

Instalar las librerias:

 - Wire
 - EspMQTTClient

##Para cargar programas al ESP01

 1. Verificar que el pin 5 (GPIO0) esté conectado a tierra (colocar el switch correspondiente a tierra) para programarlo. 
 2. Presionar el botón de reset (push button para el pin 6). Aparece un mensaje en el puerto serial del IDE del Arduino a 74880 baud que indica que está en "boot mode".
 3. Cargar el programa. 

##Para ejecutar programas en el ESP01

 1. Al correr el programa, conectar el pin 5 (GPIO0) al ADS1115. 
 2. Presionar el botón de reset (push button para el pin 6). Aparece un mensaje en el puerto serial del IDE del Arduino a 74880. 
 3. El programa debe ejecutarse.