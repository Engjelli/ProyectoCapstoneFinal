# Clase: conexionMQTT
# Esta clase se encarga de conectarse por protocolo MQTT para recibir y publicar mensajes
# Por Fernando Daniel Ramirez

import paho.mqtt.client as mqtt
from manejoJSON import manejoJSON
from extraerdatosDB import extraerdatosDB
from datetime import datetime,timedelta
import math


class conexionMQTT():
    
    # El constructor recibe los datos para conectarse a un broker a traves del protocolo MQTT
    def __init__(self,addr,usr,passw,topic):
        self.MQTT_ADDRESS = addr
        self.MQTT_USER = usr
        self.MQTT_PASSWORD = passw
        self.MQTT_TOPIC = topic

        self.mqtt_client = mqtt.Client()
        self.mqtt_client.username_pw_set(self.MQTT_USER, self.MQTT_PASSWORD)
        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message

        self.mqtt_client.connect(self.MQTT_ADDRESS, 1883)
        self.mqtt_client.loop_forever()

    # Se suscribe a un tema
    def on_connect(self,client, userdata, flags, rc):
        """ The callback for when the client receives a CONNACK response from the server."""
        print('Connected with result code ' + str(rc))
        client.subscribe(self.MQTT_TOPIC)

    # Cuando llega un mensaje en el tema suscrito, se ejecuta este metodos
    def on_message(self,client, userdata, msg):
        """The callback for when a PUBLISH message is received from the server."""
        print(msg.topic + ' ' + str(msg.payload))
        self.obtener_datos(msg.payload) # Obtiene los datos solicitados

    # Este metodo obtiene los datos solicitados, segun los parametros del mensaje recibido
    def obtener_datos(self,paramjson):
        # Parse json
        mjson = manejoJSON() # Crea un objeto para leer objetos JSON
        extrDB = extraerdatosDB() # Crea un objeto para extraer datos de la base de datos
        
        parametrosdict = mjson.convertirJSON(paramjson) # Lee el objeto JSON y lo convierte en un diccionario de Python
        #print(parametrosdict)

        # Extrae los datos solicitados segun el modo del mensaje recibido
        if parametrosdict["mode"] != "TABLA": # Extrae datos de la base de datos
            str_p = ""
            str_e = ""

            if parametrosdict["mode"] == "MES": # Modo para calcular consumo electrico mensual
                #print("modo Mes")
                #print(parametrosdict)

                # Se calcula la fecha del primer dia del mes y el intervalo de tiempo que dura ese mes
                today = datetime.today()
                fecha = datetime(today.year,parametrosdict["fecha"],1,0,0,0)
                #print(fecha)
                if parametrosdict["fecha"]+1 > 12:
                    fecha2 = datetime(today.year+1,1,1,0,0,0)
                else:
                    fecha2 = datetime(today.year,parametrosdict["fecha"]+1,1,0,0,0)
                delta = fecha2 - fecha
                deltatime = delta.days * 24 * 60
                #print(deltatime)
                
                str_p,str_e = extrDB.extraerdatos(parametrosdict["nombre"],"MES",fecha,deltatime,"ADD") # Se extraen datos de la base de datos
                print(str_e)
                self.mqtt_client.publish("capstone_energia/consumoEnergia",str_e) # Se publica un mensaje por MQTT con la informacion requerida
                extrDB.connDB.conn.close() # Se cierra la conexion con la base de datos
                return
            elif parametrosdict["mode"] == "DIFF": # Extrae los datos mas recientes
                str_p,str_e = extrDB.extraerdatos(parametrosdict["nombre"],"FIN",-1,abs(round(float(parametrosdict["deltatime"]))),parametrosdict["mode"]) # Se extraen datos de la base de datos
                
            elif parametrosdict["mode"] == "ADD": # Extrae los datos mas antiguos
                str_p,str_e = extrDB.extraerdatos(parametrosdict["nombre"],"INI",-1,abs(round(float(parametrosdict["deltatime"]))),parametrosdict["mode"]) # Se extraen datos de la base de datos
                
            elif parametrosdict["mode"] == "FECHA": # Extrae datos cercanos a una fecha dada
                #print("modo fecha")

                # Se corrige el horario de la fecha recibida
                delta_hour = timedelta(hours=6)

                date = datetime.fromtimestamp(math.floor(int(parametrosdict["fecha"]))/1000)
                hour = datetime.fromtimestamp(math.floor(int(parametrosdict["hora"]))/1000) + delta_hour
                fecha = datetime(date.year,date.month,date.day,hour.hour,hour.minute,hour.second,0)
                deltatime = round(float(parametrosdict["deltatime"]))

                # Se determina si el intervalo es despues o antes de la fecha recibida
                addifmode = ""
                if deltatime > 0:
                    adddifmode = "ADD"
                else:
                    deltatime = abs(deltatime)
                    adddifmode = "DIFF"

                #print("date: " + str(date))
                #print("hour: " + str(hour))
                print("fecha: " + str(fecha))
                
                str_p,str_e = extrDB.extraerdatos(parametrosdict["nombre"],"FECHA",fecha,deltatime,adddifmode) # Se extraen datos de la base de datos

            self.mqtt_client.publish("capstone_energia/datosPotencia",str_p) # Se publica un mensaje por MQTT con un JSON de las potencias extraidas
            self.mqtt_client.publish("capstone_energia/datosEnergia",str_e) # Se publica un mensaje por MQTT con un JSON de las energias extraidas
            extrDB.connDB.conn.close() # Se cierra la conexion con la base de datos
            
        else: # Obtiene los nombres de las tablas de la base de datos
            table_names = extrDB.extraernombretablas() # Se extraen los nombres de las tablas de la base de datos
            print(table_names)
            self.mqtt_client.publish("capstone_energia/nombreTablas",str(table_names)) # Se publica un mensaje por MQTT con un JSON de los nombres de las tablas de la base de datos
            
        
        
if __name__ == '__main__': # Un ejemplo del funcionamiento de esta clase
    print('MQTT ')
    connMQTT = conexionMQTT("3.126.191.185","fernando","pass1234","capstone_energia/parametrosGraficas")

