# Clase: manejoJSON
# Esta clase se encarga convertir los objetos JSON en diccionarios de python
# Por Fernando Daniel Ramirez


#Ejemplo de JSON
#{"nombre":"DELL_PC1","fecha":"1645110032167","hora":"1645110032167","deltatime":"null","mode":"DIFF"}

import json


class manejoJSON():
    def __innit(self):
        pass

    # Convierte un objeto JSON en un diccionario de python
    def convertirJSON(self,parametrosjson):

        parametrosdict = json.loads(parametrosjson)

        #print(parametrosjson)
        #print(parametrosdict)

        return parametrosdict

if __name__ == '__main__': # Un ejemplo del funcionamiento de esta clase
    mjson = manejoJSON()
    mjson.convertirJSON('{"nombre":"DELL_PC1","fecha":"1645110663261","hora":"1645110663261","deltatime":"10","mode":"DIFF"}')
