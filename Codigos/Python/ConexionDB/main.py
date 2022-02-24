
#from extraerdatosDB import extraerdatosDB
from conexionMQTT import conexionMQTT

# Se realiza una conexion MQTT para recibir mensajes provenientes de NodeRed
connMQTT = conexionMQTT("3.126.191.185","fernando","pass1234","capstone_energia/parametrosGraficas")
