# Clase: ConexionDB
# Esta clase se encarga de establecer una conexion con la base de datos y formular las querys para extraer datos de la base de datos
# Por Fernando Daniel Ramirez

import mariadb
from datetime import datetime, timedelta
import matplotlib.dates as mdates


class conexionDB():

    # El constructor recibe los datos para conectarse a una base de datos
    def __init__(self,usr,passw,host,port,database):
        try:
            self.conn = mariadb.connect(
                user = usr,
                password = passw,
                host = host,
                port = port,
                database = database,
            )
            #print("Conector from database "+database)
        except mariadb.Error as e:
            print(f"Error connecting to MariaDB Platform: {e}");
            sys.exit(1)

    # Extrae datos de la base de datos segun los parametros recibidos
    def obtenerdatos(self,table,date,interval,mode):
        cur = self.conn.cursor()

        # Determina si el intervalo de tiempo es antes o despues de la fecha recibida, segun el modo indicado
        date1 = ""
        date2 = ""
        
        if mode == "ADD":
            #print("modo add")
            date1 = date
            date2 = date + interval
        elif mode == "DIFF":
            #print("modo diff")
            date1 = date - interval
            date2 = date
        else:
            print("Modo invalido")
            return
        #print("date1: " + str(date1))
        #print("date2: " + str(date2))
        
        print("SELECT fecha,potencia,energia FROM " + str(table) + " WHERE fecha BETWEEN '" + str(date1) + "' AND '" + str(date2) + "'")
        cur.execute("SELECT fecha,potencia,energia FROM " + str(table) + " WHERE fecha BETWEEN '" + str(date1) + "' AND '" + str(date2) + "'") # Se ejecuta la query
        fechas, potencias, energias = self.extraerdatosDB(cur) # Extrae los datos de la query

        return fechas,potencias,energias # Regresa las listas de los datos extraidos

    # Extrae los nombres de las tablas de la base de datos
    def extraernombrestablas(self):
        cur = self.conn.cursor()
        cur.execute("SELECT table_name FROM information_schema.tables WHERE table_schema = \'" + self.conn.database + "\'") # Se ejecuta la query


        # Se guardan los nombres de las tablas en una lista
        table_names = []
        
        for table_name in cur:
            for name in table_name:
                table_names.append(name)
        
        #print(table_names)
        
        return table_names # Regresa lalistas de los nombres de las tablas de la base de datos

    # Extrae datos de la base de datos
    def extraerdatosDB(self,cur):

        # Guarda los datos extraidos en listas
        
        fechas = []
        potencias = []
        energias = []
        
        for (fecha, potencia, energia) in cur:
            fechas.append(datetime.strptime(f"{fecha}", '%Y-%m-%d %H:%M:%S'))
            #print(fecha)
            #print(type(fecha))
            
            potencias.append(float(f"{potencia}"))
            energias.append(float(f"{energia}"))

        #print("Fecha: " + str(fechas[len(fechas)-1]) + ", Potencia: " + str(potencias[len(potencias)-1]) + ", Energia: " + str(energias[len(energias)-1]))
        
        return fechas,potencias,energias # Regresa las listas de los datos obtenidos

    # Obtiene la fecha mas cercana a la almacenada en la base de datos
    def obtenerfecha(self,table,date,mode):
        cur = self.conn.cursor()
        #print("mode:" + str(mode))
        #print("type(mode):" + str(type(mode)))
        #print("mode == INI:" + str(mode == "INI"))

        
        if mode == "FIN": # Obtiene la ultima fecha registrada en la base de datos
            #print("modo fin")
            cur.execute("SELECT potencia,max(fecha) FROM " + table)
            
        elif mode == "INI": # Obtiene la primera fecha registrada en la base de datos
            #print("modo ini")
            cur.execute("SELECT potencia,min(fecha) FROM " + table)
        elif mode == "FECHA": # Obtiene la fecha mas cercana registrada en la base de datos
            #print("modo fecha")
            return self.verificarfecha(cur,table,date) # Verifica si la fecha dada esta entre la primera y ultima fecha registrada en la base de datos
        elif mode == "MES": # Para este modo no se realiza ninguna modificacion
            return date
            #print("modo mes")
        else:
            print("Modo no valido")
            return ""
        
        fecha_obtenida = ""

        # Se extrae la fecha mas cercana registrada en la base de datos
        for (potencia,fecha) in cur:
            #print(fecha)
            #print(type(fecha))
            #print(fecha is None)
            if fecha is None: # En caso de no haber registros de la base de datos, se regresa la fecha correspondiente al dia de hoy
                #print("fecha is null")
                today = datetime.now()
                fecha_obtenida = datetime(today.year,today.month,today.day,0,0,0)
            else:
                #print("fecha valida")
                fecha_obtenida = datetime.strptime(f"{fecha}", '%Y-%m-%d %H:%M:%S')
            #print(fecha)
        
        #print("fecha_obtenida: " + str(fecha_obtenida))

        return fecha_obtenida # Regresa la fecha obtenida

    
    # Verifica que la fecha recibida este entre la primera y ultima fecha almacenadas en la base de datos
    def verificarfecha(self,cur,table,date):

        # Obtiene la primera y ultima fecha registradas en la base de datos
        primera_fecha = self.obtenerfecha(table,-1,"INI")
        ultima_fecha = self.obtenerfecha(table,-1,"FIN")
        if date <= primera_fecha: # En caso de que la fecha recibida sea anterior a la primera fecha registrada de la base de datos, regresa la primera fecha
            return self.obtenerfecha(table,-1,"INI")
        elif date >= ultima_fecha: # En caso de que la fecha recibida sea superior a la ultima fecha registrada de la base de datos, regresa la ultima fecha
            return self.obtenerfecha(table,-1,"FIN")
        else: # En caso de que la fecha recibida este entre la primera y ultima fecha registrada en la base de datos, se calcula la fecha mas cercana almacenada en la base de datos
            delta_date = self.obtenerdeltafecha(0,0,15,0)
            date1 = date - delta_date
            date2 = date + delta_date
            cur.execute("SELECT fecha,potencia FROM " + str(table) + " WHERE fecha BETWEEN '" + str(date1) + "' AND '" + str(date2) + "'")
            
            #fecha_obtenida = ""

            #Se extrae la fecha mas cercana registrada en la base de datos
            for (fecha,potencia) in cur:
                fecha_obtenida = datetime.strptime(f"{fecha}", '%Y-%m-%d %H:%M:%S')
                if date - fecha_obtenida < timedelta(seconds=0):
                    #print("fecha_obtenida: " + str(fecha_obtenida))
                    return fecha_obtenida
                
        return self.obtenerfecha(table,-1,"FIN")
                
    # Obtiene un delta de tiempo dados los parametros
    def obtenerdeltafecha(self,day,hour,minute,second):
        delta_fecha = timedelta(days=day,hours=hour,minutes=minute,seconds=second)
        return delta_fecha



        
