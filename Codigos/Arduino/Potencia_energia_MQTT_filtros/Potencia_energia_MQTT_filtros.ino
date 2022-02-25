/*
 * Este programa realiza el calculo de potencia de un dispositivo a partir de las mediciones de voltaje y corriente
 * 
 * Por Fernando Daniel Ramírez Cruz
 * Basado en el progamas de: 
 * Lewis Loflin. Tomado de https://www.bristolwatch.com/arduino2/ard0.htm
 * Hugo Escalpelo. Tomado de https://github.com/hugoescalpelo/ESP32CAM-WiFi-Basic/blob/main/ESP32CAM-WiFi-Basic/ESP32CAM-WiFi-Basic.ino
 */

/*
 * DESCRIPCION DEL PROGRAMA:
 * 
 * 
 * Cada 870 us se toma una muestra, ya sea de voltaje o corriente
 * 
 * La frecuencia de la red electrica es de 60Hz o 16.6 ms
 * Por cada ciclo (cada 16.6 ms) se toman 18 muestras (18 * 870 us = 15.66 ms, que es menor que 1/60 s)
 * 
 * Por cada 4/60 de segundo (66.6 ms para 4 ciclos de 60Hz) se toman (18*4)-2=74 muestras
 * Cada 66 ms se realiza un calculo de potencia y se alterna entre lectura de muestras de voltaje y corriente 
 * Es decir, 66.6 ms para tomar muestras de corriente y 66.6 ms para tomar muestras de voltaje. Este proceso se repite indefinidamente
 * Se realizan 15 calculos de potencia por segundo. 
 * 
 * Cada 10 segundos se realiza un promedio de los 150 calculos de potencia realizados
 * 
 * Para calcular la potencia se realiza lo siguiente:
 * Identificar la muestra con el valor mayor y con el valor menor
 * Elevar al cuadrado cada muestra de voltaje y sumarlas. Elevar al cuadrado cada muestra de corriente y sumarlas
 * Promediar la suma de cuadrados de voltaje. Promediar la suma de cuadrados de corriente
 * Vrms = Sacar la raiz cuadrada del promedio de la suma de cuadrados de voltaje. Irms = Sacar la raiz cuadrada del promedio de la suma de cuadrados de corriente. 
 * Realizar P = Vrms * Irms / 2, donde P es la promedio de la protencia que consume ese dispositivo
 * 
 * Adicionalmente se calcula la energia en Joules:
 * Multiplicar la muestra de potencia calculada por el tiempo que dura esa muestra (4/60 de segundo o 1 segundo entre 15 muestras)
 * Sumar todos los calculos de energia por 10 segundos
 * 
 * 
 * NOTAS ADICIONALES
 * 
 * Tiempo minimo para realizar una medicion: 820 - 830 us sin utilizar la libreria ADS11X5.h
 * NO UTILIZAR LA LIBRERIA ADS11X5.h ya que es muy lenta, tarda alrededor de 10ms en tomar una muestra
 * Se requiere un minimo de 8.3 ms para tomar al menos 2 muestras por ciclo
 * 
 * Las señales leidas del ADS1115 tienen una componente de voltaje de offset equivalente a Vcc/2, donde Vcc es el voltaje de alimentacion que se utiliza para alimentar el sensor de voltaje y el sensor de corriente
 * Puesto que para el voltaje se identifica la muestra con valor maximo y minimo, se puede calcular el valor medio de esta manera: voltaje_offset = (maximo - minimo)/2 + minimo
 * Para obtener la lectura verdadera de cada muestra que se mide de los sensores, se realiza: (medicion - voltaje_offset) * ganancia
 * 
 * La ganancia de voltaje se determino expermientalmente para el sensor ZMPT101B y puede variar entre 800 y 1100 aproximadamente (incluye la ganancia por el filtro RC)
 * La ganancia de corriente depende de la resistencia que se utilice para el sensor SCT013(100A/50mA) 
 * Como este sensor SCT013 da una salida de corriente, la resistencia utilizada convierte esta corriente en voltaje
 * La ganancia de corriente, utilizando una resistencia de 1Kohm es de apromadamente 2 (2000/1000)
 * 
 */


//Bibliotecas
#include <Wire.h> // Biblioteca para la comunicacion I2C
#include <ESP8266WiFi.h>  // Biblioteca para el control de WiFi
#include <PubSubClient.h> //Biblioteca para conexion MQTT

//Datos de WiFi
const char* ssid = "INFINITUMBF13_2.4";  // Aquí debes poner el nombre de tu red
const char* password = "0007213411";  // Aquí debes poner la contraseña de tu red


//Datos del broker MQTT
const char* mqtt_server = "3.126.191.185"; // Si estas en una red local, coloca la IP asignada, en caso contrario, coloca la IP publica

// para ubicar ip, ejecutar en una terminal nslookup broker.hivemq.com y colocar ip publica o privada
IPAddress server(3,126,191,185);

// Objetos
WiFiClient espClient; // Este objeto maneja los datos de conexion WiFi
PubSubClient client(espClient); // Este objeto maneja los datos de conexion al broker


//Variables para medir tiempo
unsigned long t_millis, t_micros, t_medicion, t_mediciones_tomadas, t_ciclo, t_calculo_potencia;
unsigned int count = 0;

//Constantes de tiempo
const int t_por_calculo_potencia = 10000000; // Cada 10 segundos se calcula un promedio de las potencias calculadas (en microsegundos)
const int t_por_ciclo = 16667; // Frecuencia de la señal electrica 60Hz (en microsegundos)
const int t_por_muestra = 870; // Tomar 19 muestras por ciclo (en microsegundos)

//Constantes para la tma de muestras
const int num_muestras = 18; // Muestras tomadas por ciclo (por cada 1/60 de segundo o 870us aproximadamente)
const int num_ciclos = 4; // Numero de ciclos para tomar voltaje o corriente
const int num_muestras_tomadas = (num_muestras * num_ciclos) - 2; // Total de muestras tomadas por cada num_ciclos. Las ultimas dos muestras no se toman. 

// variables para la toma de muestras
double muestras[num_muestras_tomadas] = {}; // arreglo con las muestras tomadas

// variables para el calculo de potencia
double potencia = 0; // calculo de potencia vrms*irms
double potencia_acc = 0; // acumula calculos de potencia
double potencia_prom = 0; // promedio de varios calculos de potencia

double energia = 0; // calculo de energia P*t
double energia_acc = 0; // acumula calculos de energia

double voffset = 2.5; // voltaje de offset sobre la que estan montadas las señales de voltaje y corriente
const double v_ganancia = 915; // ganancia del sensor de voltaje
const double i_ganancia = 2000/551; // ganancia del sensor de corriente

double maximo[2], minimo[2]; // Almacenan la muestra con valor maximo y con valor minimo
double v2 = 0, i2 = 0; // Muestra de voltaje al cuadrado y corriente al cuadrado
double vrms = 0, irms = 0; // voltaje rms y corriente rms
double voltaje = 0,corriente = 0; // Almacenan el valor de voltaje y corriente medido

byte i_muestra = 0, i_potencia = 0; // Iteradores para la toma de muestras y para los calculos de potencia
byte corriente_o_voltaje = 0; // indica si se debe tomar corriente o voltaje
byte corriente_offset = 0; 
double ioffset = 0;
double v_acc = 0;

byte i_num_muestras = 0;


// variables para la lectura del ADC
unsigned int val = 0; // Almacena la lectura del ADC
byte muestras_listas = 0; // Indica si todas las muestras ya fueron tomadas

const byte ASD1115 = 0x48; // Direccion I2C del ADC
const double VPS = 6.144 / 32768.0; // volts por paso

byte writeBuf[3]; // variable para escribir en el ADC
byte buffer[3]; // variable para recibir datos del ADC

/*
 * Se inicializa la comunicacion serial, la comunicacion i2c, el ADC y las variables de tiempo. 
 * 
 */
void setup()   {

  Serial.begin(115200); // Inicia el monitor serial a 115200 baud
  Wire.begin(0,2); // Inicializa la comunicacion I2C en los pines GPIO0 y GPIO2

  Serial.println();
  
  inicializar_canal_ADC(); // Inicializa un canal del ADC

  delay(1000);
  
  setup_wifi(); // Inicializa el Wifi y servidor MQTT
  client.setServer(server, 1883); // Conectarse a la IP del broker en el puerto indicado
  client.setCallback(callback); // Activar función de CallBack, permite recibir mensajes MQTT y ejecutar funciones a partir de ellos
  
  delay(1000);

  // Inicializa variables de tiempo
  t_micros = micros();
  t_millis = millis();
  t_mediciones_tomadas = micros();
  t_ciclo = micros();
  t_calculo_potencia = millis();
}  // end setup


void loop() {
  mediciones_potencia();
} // end loop

void mediciones_potencia(){
  
  medir_voltaje_corriente(); // Realiza mediciones de voltaje y corriente
  
  if((micros() - t_calculo_potencia) > t_por_calculo_potencia)
  {
    potencia_prom = potencia_acc / i_potencia; // Calcula la potencia promedio de las 150 muestras

    // Si por alguna razon las muestras fueron diferentes de 150, se realiza un ajute al calculo de energia
    if(i_potencia != 150)
    {
      energia_acc = energia_acc*150/i_potencia;
    }

    // Imprime potencia promedio, energia y numero de meustras
    Serial.print("Potencia (Vrms*Irms/2): "); Serial.println(potencia_prom);
    Serial.print("Energia (sum(P*t)): "); Serial.println(energia_acc);
    Serial.print("Muestras: "); Serial.println(i_potencia);
    //delay(2000);


    enviar_datos(); // envia los datos por mqtt

    // Limpia e inicializa variables
    energia_acc = 0;
    potencia_acc = 0;
    energia_acc = 0;
    
    i_potencia = 0;
    t_calculo_potencia = micros();
    t_ciclo = micros();
    t_mediciones_tomadas = micros();
  }
}


/**
 * Envia los datos de id de la maquina, potencia promedio y energia a traves del protocolo mqtt, utilizando un broker
 */
void enviar_datos(){
  //Verificar siempre que haya conexión al broker
  if (!client.connected()) {
    reconnect();  // En caso de que no haya conexión, ejecutar la función de reconexión, definida despues del void setup ()
  }// fin del if (!client.connected())
  client.loop(); // Esta función es muy importante, ejecuta de manera no bloqueante las funciones necesarias para la comunicación con el broker


  //Se construye el string correspondiente al JSON que contiene 3 variables
  String json = "{\"id\":\"HP_PC1\",\"potencia\":"+String(potencia_prom)+",\"energia\":"+String(energia_acc)+"}";
  int str_len = json.length() + 1;//Se calcula la longitud del string
  char char_array[str_len];//Se crea un arreglo de caracteres de dicha longitud
  json.toCharArray(char_array, str_len);//Se convierte el string a char array    

  Serial.print("json enviado: "); Serial.println(char_array); // Se imprime en monitor solo para poder visualizar que el evento sucede
  
  client.publish("capstone_energia/potencia", char_array); // Esta es la función que envía los datos por MQTT, especifica el tema y el valor
}

/*
 * Esta funcion toma mediciones por num_ciclos y despues calcula la potencia del conjunto de meustras
 * 
 */
void medir_voltaje_corriente(){
  if((micros() - t_ciclo) < t_por_ciclo * num_ciclos && muestras_listas == 0)
  {
    tomar_mediciones(); // Se toma un total de muestras de num_muestras_tomadas 
  }
  else
  {
    if(muestras_listas == 1 && (micros() - t_ciclo) > ((t_por_ciclo * num_ciclos) - 3600))
    {
      //Serial.print("Nuevo ciclo "); Serial.print(i_potencia); Serial.print(" : "); Serial.println(micros() - t_ciclo);

      obtener_maximo_minimo(); // Se obtiene el maximo y minimo del conjunto de muestras tomadas
      calcular_potencia(); // Se calcula la potencia del conjunto de muestras tomadas
      calcular_energia();
      //imprimir_parametros();
      
      //delay(910);

      if(corriente_o_voltaje == 0)
      {
        corriente_o_voltaje = 1; // Se mide voltaje para el siguiente conjunto de muestras
      }
      else
      {
        corriente_o_voltaje = 0; // Se mide corriente para el siguiente conjunto de muestras
        corriente_offset = 1;
        inicializar_canal_ADC();
        //int micros_aux = micros();
        //Serial.print("micros ini: "); Serial.println(micros_aux);
        for(int i = 0; i < 5; i++)
        {
          ioffset = leer_ADC() * VPS;
          //Serial.print("offset corriente: "); Serial.println(ioffset);
        }
        //Serial.print("t ioffset: "); Serial.println(micros() - micros_aux);

        //delay(2000);
        i_num_muestras = 0;
        corriente_offset = 0;
      }
      
      //Serial.print("c_o_v: "); Serial.println(corriente_o_voltaje);
      inicializar_canal_ADC(); // Se inicializa el canal del ADC para medir corriente o voltaje segun la variable corriente_o_voltaje
      
      i_potencia ++; // Incrementa el contador para calculos de potencia
      t_ciclo = micros(); // reinicia la variable de tiempo correspondiente
      t_mediciones_tomadas = micros();
      muestras_listas = 0; // limpia la bandera para volver a tomar un conjunto de muestras
    }
  }
}

/*
 * Esta funcion toma una muestra del ADC
 * 
 */
void tomar_mediciones(){
  if((micros() - t_medicion) > t_por_muestra)
  {
    if(i_muestra < num_muestras_tomadas)
    {
      //Serial.print("Muestra tomada "); Serial.print(i_muestra); Serial.print(" ");
      //imprimir_medicion_y_tiempo_en_micros(val);
      t_medicion = micros();
      val = leer_ADC();
      
      muestras[i_muestra] = val * VPS;
      muestras_listas = 0;
      i_muestra++;
    }
    else
    {
      //Serial.println("Muestras de la señal");
      /*for(int i = 0; i < num_muestras_tomadas; i++)
      {
        Serial.print(i); Serial.print(": "); Serial.println(muestras[i]);
      }*/

      //Serial.print("Muestras tomadas: "); Serial.println(i_muestra);
      //Serial.print("t: "); Serial.println((micros() - t_mediciones_tomadas));
      //delay(1000);
      
      i_muestra = 0;
      muestras_listas = 1;
      t_mediciones_tomadas = micros();
    }
  }
}

/**
 * Esta funcion calcula la potencia a partir de las mediciones de voltaje y corriente
 * 
 */
void calcular_potencia(){
  //voffset = v_acc/(num_muestras_tomadas - 4);

  vrms = sqrt(v2/(num_muestras_tomadas - 4));
  irms = sqrt(i2/(num_muestras_tomadas - 4));

  potencia = vrms * irms;
  potencia_acc += potencia;

  //Serial.print("potencia: "); Serial.println(potencia);
  //imprimir_parametros();
}


/**
 * Esta funcion calcula la energia a partir del calculo de potencia
 * 
 */
void calcular_energia(){
  energia = potencia * t_por_calculo_potencia/(150*1000000);
  energia_acc += energia;
}

/*
 * Esta funcion obtiene el maximo y minimo de las muestras tomadas y realiza el calculos para la potencia,
 * encontrando el voltaje al cuadrado y la corriente al cuadrado
 * 
 */
void obtener_maximo_minimo(){
  // Se inicializa las variables de los maximo y minimo para corriente (corriente_o_voltaje = 0) o voltaje (corriente_o_voltaje = 1)
  maximo[corriente_o_voltaje] = -1.0;
  minimo[corriente_o_voltaje] = 6.0;

  //Serial.print("c_o_v max min: "); Serial.println(corriente_o_voltaje);
  if(corriente_o_voltaje == 0)
  {
    //Serial.println("Corriente");
    i2 = 0; // Se inicializa la variable de corriente al cuadrado
  }
  else
  {
    //Serial.println("Voltaje");
    v2 = 0; // Se inicializa la variable de voltaje al cuadrado
    v_acc = 0;
    for(int i = 4; i < num_muestras_tomadas - 4; i++)
    {
      v_acc += muestras[i];
    }
    voffset = v_acc/(num_muestras_tomadas - 8);
  }

  

  
  for(int i = 4; i < num_muestras_tomadas - 4; i++)
  {
    // Se revisa para el valor maximo del conjunto de muestras
    if(muestras[i] > maximo[corriente_o_voltaje])
    {
      maximo[corriente_o_voltaje] = muestras[i]; 
    }

    // Se revisa para el valor minimo del conjunto de muestras
    if(muestras[i] < minimo[corriente_o_voltaje])
    {
      minimo[corriente_o_voltaje] = muestras[i];
    }

    
    if(corriente_o_voltaje == 0)
    {
      // Se realiza el calculo para la corriente al cuadrado
      corriente = (muestras[i] - ioffset) * i_ganancia; // Se resta la componente de offset y se multiplica por la ganancia
      i2 += pow(corriente,2); // Se calcula la corriente al cuadrado
      //Serial.print("corriente "); Serial.print(i); Serial.print(": "); Serial.println(corriente); 
    }
    else
    {
      // Se realiza el calculo para el voltaje al cuadrado
      voltaje = (muestras[i] - voffset) * v_ganancia; // Se resta la componente de offset y se multiplica por la ganancia

      v2 += pow(voltaje,2); // Se calcula el voltaje al cuadrado
    }
    
    muestras[i] = 0; // Se limpia el arreglo para la siguiente toma de muestras
  }
}

/*
 * Esta funcion inicializa uno de los canales del ADC para tomar muestras
 * Se escriben tres bytes al ADC:
 * Byte 1: Se indica que se escribe al registro de configuracion
 * Byte 2: Se especifican los bits 15-8 del registro de configuracion
 * Byte 3: Se especifican los bits 7-0 del registro de configuracion
 * 
 */
void inicializar_canal_ADC(){

  writeBuf[0] = 1; // Puntero al registro de configuracion que es 1

  if(corriente_o_voltaje == 0)
  {
    if(corriente_offset == 0)
    {
      //Serial.println("Corriente mediciones");
      writeBuf[1] = 0b11000000; // Bits 15-8: Inicializa el pin A0, en donde esta el sensor de corriente
      // 0xC0: Modo Single Shot / AIN 0 & GND / 6.144V / Modo Continuo
    }
    else
    {
      //Serial.println("Corriente offset");
      writeBuf[1] = 0b11100000; // Bits 15-8: Inicializa el pin A2 para medir offset del sensor de corriente
      // 0xE0: Modo Single Shot / AIN 2 & GND / 6.144V / Modo Continuo
    }

  }
  else
  {
    //Serial.println("Voltaje mediciones");
    writeBuf[1] = 0b11010000; // Bits 15-8: Inicializa el pin A1, en donde esta el sensor de voltaje
    // 0xD0: Modo Single Shot / AIN 1 & GND / 6.144V / Modo Continuo
  }
  
  //Serial.print("pin: "); Serial.println(writeBuf[1],HEX);
  
  // bit 15 flag bit for single shot
  // Bits 14-12 input selection:
  // 100 ANC0; 101 ANC1; 110 ANC2; 111 ANC3 (Single end)
  // Bits 11-9 Amp gain. Default to 010 here 001 P19
  // Bit 8 Operational mode of the ADS1115.
  // 0 : Continuous conversion mode
  // 1 : Power-down single-shot mode (default)

  // 000 // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // 001 // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // 010 // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // 011 // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // 100 // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // 101 - 111 // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

  writeBuf[2] = 0b11100101; //Bits 7-0: 0xE5 / 860 SPS 
  
  // Bits 7-5 data rate default to 111 for 860SPS
  // Bits 4-0 comparator functions see spec sheet. Para este caso no aplica

  // Inicializar el ADS1115
  // Escribir al registro de configuracion
  Wire.beginTransmission(ASD1115);  // ADC 
  Wire.write(writeBuf[0]); // pointer to config register which is 1
  Wire.write(writeBuf[1]); // MSB of config register
  Wire.write(writeBuf[2]); // LSB of config register
  Wire.endTransmission();
}

/*
 * Esta funcion lee un valor del ADC. 
 * Se envia 1 byte indicando solicitando la lectura de un canal
 * Se reciben 2 bytes con esa lectura
 * 
 */
int leer_ADC(){
  int val = 0;
  buffer[0] = 0; // Puntero al registro de conversion que es 0

  // write to pointer register
  Wire.beginTransmission(ASD1115);  // DAC
  Wire.write(buffer[0]);  // Points to conversion register which is 0
  Wire.endTransmission();

  // read conversion register
  Wire.requestFrom(ASD1115, 2);
  buffer[1] = Wire.read();  // MSB(Byte Mas Significativo, bits 15-8) of conversion register
  buffer[2] = Wire.read();  // LSB(Byte Menos Significativo, bits 7-0) of conversion register
  Wire.endTransmission();  

  // convert display results
  val = buffer[1] << 8 | buffer[2]; // Se juntan los dos bytes para obtener la medicion
  
  if (val > 32768) val = 0; // En caso de una lectura erronea, se toma como 0

  i_num_muestras ++;
  
  return val;
}

/*
 * Esta función inicializa el WiFi 
 * 
 */
void setup_wifi() {
 
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Esta función permite tomar acciones en caso de que se reciba un mensaje correspondiente a un tema al cual se hará una suscripción
void callback(char* topic, byte* message, unsigned int length) {

  // Indicar por serial que llegó un mensaje
  Serial.print("Llegó un mensaje en el tema: ");
  Serial.print(topic);

  // Concatenar los mensajes recibidos para conformarlos como una varialbe String
  String messageTemp; // Se declara la variable en la cual se generará el mensaje completo  
  for (int i = 0; i < length; i++) {  // Se imprime y concatena el mensaje
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  // Se comprueba que el mensaje se haya concatenado correctamente
  Serial.println();
  Serial.print ("Mensaje concatenado en una sola variable: ");
  Serial.println (messageTemp);

  // En esta parte puedes agregar las funciones que requieras para actuar segun lo necesites al recibir un mensaje MQTT

  // Ejemplo, en caso de recibir el mensaje true - false, se cambiará el estado del led soldado en la placa.
  // El ESP323CAM está suscrito al tema esp/output
  if (String(topic) == "esp32/outputfdrc") {  // En caso de recibirse mensaje en el tema esp32/outputfdrc
    if(messageTemp == "true"){
      Serial.println("Led encendido");
    }// fin del if (String(topic) == "esp32/outputfdrc")
    else if(messageTemp == "false"){
      Serial.println("Led apagado");
    }// fin del else if(messageTemp == "false")
  }// fin del if (String(topic) == "esp32/outputfdrc")
}// fin del void callback

// Función para reconectarse
void reconnect() {
  // Bucle hasta lograr conexión
  while (!client.connected()) { // Pregunta si hay conexión
    Serial.print("Tratando de contectarse...");
    // Intentar reconexión
    if (client.connect("ESP32CAMClient")) { //Pregunta por el resultado del intento de conexión
      Serial.println("Conectado");
      client.subscribe("esp32/outputfdrc"); // Esta función realiza la suscripción al tema
    }// fin del  if (client.connect("ESP32CAMClient"))
    else {  //en caso de que la conexión no se logre
      Serial.print("Conexion fallida, Error rc=");
      Serial.print(client.state()); // Muestra el codigo de error
      Serial.println(" Volviendo a intentar en 5 segundos");
      // Espera de 50 milisegundos bloqueante
      delay(5000);
      Serial.println (client.connected ()); // Muestra estatus de conexión
    }// fin del else
  }// fin del bucle while (!client.connected())
}// fin de void reconnect(


/*
 * Imprime la medicion tomada y el tiempo que se tardo en tomar la medicion
 * 
 */
void imprimir_medicion_y_tiempo_en_micros(int val){

  //Serial.print((micros() - t_micros));
  //Serial.print(" ");
  Serial.println(val * VPS);
  
  //t_millis = millis();
  t_micros = micros();

  
}

/*
 * Imprime el numero de ejecuciones de la funcion void loop en 1 segundo
 */
void imprimir_ejecuciones_por_segundo(){
  
  if((millis() - t_millis) > 1000){
    Serial.print("c : "); Serial.println(count);
    count = 0;
  }
  else
  {
    count ++;
  }
}

/*
 * Imprime diversos parametros de los calculos de potencia
 * 
 */
void imprimir_parametros(){
  //Serial.print("Corriente "); Serial.print(maximo[0]); Serial.print(" , "); Serial.println(minimo[0]);
  //Serial.print("Voltaje "); Serial.print(maximo[1]); Serial.print(" , "); Serial.println(minimo[1]);
  //Serial.print("Voltaje de offset "); Serial.println(voffset);
  //Serial.print("Corriente de offset "); Serial.println(ioffset);
  //Serial.print("vrms: "); 
  Serial.print(vrms); Serial.print(",");
  //Serial.print("irms: "); 
  Serial.println(irms,6);
  //Serial.print("Potencia "); Serial.print(i_potencia); Serial.print(": "); Serial.println(potencia);
  //Serial.print("Energia "); Serial.print(i_potencia); Serial.print(": "); Serial.print(energia); 
  //Serial.print("Num muestras: "); Serial.print(i_num_muestras); 
  //Serial.print(", t: "); Serial.println((micros() - t_ciclo));
  //Serial.println();
}
