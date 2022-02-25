# Clase: extraerdatosDB
# Esta clase se encarga de extraer datos de la base de datos
# Por Fernando Daniel Ramirez

from conexionDB import conexionDB
from graficador import graficador
from datetime import datetime
import json


class extraerdatosDB():

    # El constructor se conecta a la base de datos local con nombre "potencia"
    def __init__(self):
        self.connDB = conexionDB("fernando","pass1234","localhost",3306,"potencia") # Conector para la base de datos
        #self.graf = graficador() # Objeto para graficar datos en Python

    # Extrae datos de la base de datos, segun los parametros recibidos
    def extraerdatos(self,table,datemode,date,deltatime,adddifmode):
        # Para extraer datos, independientemente del "datemode" con el que se extraigan datos, se realizan 5 pasos.
        
        # 1. Calcular una fecha, la mas cercana a la almacenada en la base de datos
        fecha = self.connDB.obtenerfecha(table,date,datemode)

        
        #fecha = self.connDB.obtenerfecha(table,-1,"INI")
        #print("date.day: " + str(date.day))
        #print("fecha.day: " + str(fecha.day))
        #print("deltatime: " + str(deltatime))

        # Corregir si el usuario ingresa mal los datos
        if datemode == "FECHA":
            deltatime = abs(deltatime)
            if fecha == self.connDB.obtenerfecha(table,-1,"INI"):
                adddifmode = "ADD"
                pass
            elif fecha == self.connDB.obtenerfecha(table,-1,"FIN"):
                adddifmode = "DIFF"
        
        #print("deltatime: " + str(deltatime))
        #print("adddifmode: " + str(adddifmode))
        
        #print(fecha)

        # 2. Calcular un delta de tiempo en minutos para la fecha calculada
        delta_fecha = self.connDB.obtenerdeltafecha(0,0,deltatime,0) # minutos

        #print(delta_fecha)

        # 3. Extraer datos de la base de datos con los datos calculados y recibidos
        fechas,potencias,energias = self.connDB.obtenerdatos(table,fecha,delta_fecha,adddifmode)
        #fechas,potencias = self.connDB.obtenerdatos(table,fecha,delta_fecha,"ADD") # Ejemplo

        #print(str(potencias))

        # 4. Graficar datos (debug) en Python
        #self.graf.graficar_datos(fechas,potencias,'Tiempo[s]','Potencia[W]','Consumo de energia de ' + table) # Grafica los datos extraidos de potencias
        #self.graf.graficar_datos(fechas,energias,'Tiempo[s]','Energia[J]','Consumo de energia de ' + table) # Grafica los datos extraidos de energias
        
        # 5. Guardar en un archivo .txt o enviar por MQTT
        
        list_p = []
        list_e = []
        
        if datemode != "MES":
            #self.guardardatosextraidos(fechas,potencias,energias)# Guarda los datos en un archivo .txt
            list_p,list_e = self.enviardatosextraidos(fechas,potencias,energias) # Da formato a los datos extraidos como objetos JSON
        else:
            list_e = self.enviardatosextraidosmes(fechas,energias) # Da formato a los datos extraidos como objetos JSON
        
        #print(str(len(potencias)) + " dato(s) extraidos(s) de potencia")
        #print(str(len(energias)) + " dato(s) extraidos(s) de energia")

        return list_p,list_e # Regresa objetos tipo JSON

    # Extrae los nombres de las tablas de la base de datos y les da formato JSON
    def extraernombretablas(self):
        table_names = self.connDB.extraernombrestablas() # Se extraen los nombres de las tablas de la base de datos
        return json.dumps(table_names) # Se le da formato JSON

    # Obtiene el consumo mensual electrico
    def enviardatosextraidosmes(self,fechas,energias):
        energia_acc = 0
        
        for i in range(len(fechas)):
            energia_acc += (energias[i]/(3.6e+6)) #Suma todos los registros de energia para obtener el consumo total de energia

        return energia_acc # 

    # Da formato JSON a los datos extraidos de potencia y energia
    def enviardatosextraidos(self,fechas,potencias,energias):
        energia_acc = 0
        list_p = []
        list_e = []

        # Guarda los datos extraidos de potencia y energia como un arreglo de objetos JSON
        for i in range(len(fechas)):

            energia_acc += (energias[i]/(3.6e+6))

            p = {"x":str(fechas[i]),"y":potencias[i]}
            e = {"x":str(fechas[i]),"y":energia_acc}

            #p = {"x":i,"y":potencias[i]}
            #e = {"x":i,"y":energia_acc}
            
            list_p.append(p)
            list_e.append(e)
            

        return json.dumps(list_p),json.dumps(list_e) # Le da formato JSON a las listas de potencias y energias
        

    # Guarda los datos extraidos de potencia y energia en dos archivos de texto, uno para cada variable respectivamente
    def guardardatosextraidos(self,fechas,potencias,energias):
        
        f = open("potencia.txt", "w")
        g = open("energia.txt", "w")

        #print(fechas)
        #print(potencias)
        #print(energias)
        #print(type(fechas))
        #print(type(potencias))
        #print(type(energias))

        energia_acc = 0

        # Guarda objetos tipo JSON de potencia y energia respectivamente en el archivo correspondiente
        for i in range(len(fechas)):
            #strp =
            energia_acc += (energias[i]/(3.6e+6))
            f.write("{ \"fecha\": \"" + str(fechas[i]) + "\",\"potencia\": \"" + str(potencias[i]) + "\"}")
            g.write("{ \"fecha\": \"" + str(fechas[i]) + "\",\"energia\": \"" + str(energia_acc) + "\"}")
            if i < (len(fechas)-1):
                f.write("\n")
                g.write("\n")

        f.close()
        g.close()

if __name__ == '__main__': # Funcion de ejemplo
    extrDB = extraerdatosDB()
    extrDB.extraerdatos("DELL_PC1","FIN",-1,120,"DIFF")

