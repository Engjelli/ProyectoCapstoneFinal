# Clase: graficador
# Esta clase se encarga de graficar datos en python
# Por Fernando Daniel Ramirez

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as mdates


class graficador():

    def __init__(self):
        pass

    # Grafica datos en el tiempo
    def graficar_datos(self,fechas,datos,title,labelx,labely):
        fechasnum = mdates.date2num(fechas)
        formatter = mdates.DateFormatter('%H:%M:%S')
        
        #print(fechasnum)
        #print(len(fechasnum))

        plt.figure()

        ax = plt.gca()
        ax.xaxis.set_major_formatter(formatter)
        plt.setp(ax.get_xticklabels(), rotation = 30)


        plt.plot(fechasnum, datos)#, align='center', alpha=0.5)

        plt.ylabel(labely)
        plt.xlabel(labelx)
        plt.title(title)

        plt.show()
