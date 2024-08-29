##############
## Script listens to serial port and writes contents into a file
##############
## requires pySerial to be installed
import serial  #  pip install pyserial should work
import os
from datetime import datetime
import time


Sensoren_Informationen = { 
    # Sensore Name: Liste and Spalten Namen
    "GPS": ["Latitude", "Longitude", "Altitude [m]", "Geschwindigkeit [Km/h]", "Horizontale Abnahme der Genauigkeit", "Number of Satellites"],
    "particleFront": ["Particles > 0.3um / 0.1L air", "Particles > 0.5um / 0.1L air", "Particles > 1.0um / 0.1L air", "Particles > 2.5um / 0.1L air", "Particles > 5.0um / 0.1L air", "Particles > 10.0um / 0.1L air"],
    "particleBack": ["Particles > 0.3um / 0.1L air", "Particles > 0.5um / 0.1L air", "Particles > 1.0um / 0.1L air", "Particles > 2.5um / 0.1L air", "Particles > 5.0um / 0.1L air", "Particles > 10.0um / 0.1L air"],
    "particleBottom": ["Particles > 0.3um / 0.1L air", "Particles > 0.5um / 0.1L air", "Particles > 1.0um / 0.1L air", "Particles > 2.5um / 0.1L air", "Particles > 5.0um / 0.1L air", "Particles > 10.0um / 0.1L air"],
    "humid_DHT": ["humidity"],
    "Temp_DHT": ["temperature"],
}


seperatorDerNachrichtenTeileVomArduino = "|"
seperatorDesCSVVomArduino = "|"
seperatorDesCSVZumSpeichern = ";"


# Vorbereitung: Verbindung zum Arduino und Erstellen des Ordners für die Daten

serial_ports = ['COM7'] #serial_ports = ['COM7', 'COM8']
baud_rate = 57600  # In arduino, Serial.begin(baud_rate)
serials = [(port, serial.Serial(port, baud_rate)) for port in serial_ports]


# Erstelle einen neuen "Data_X" Ordenr, wobei X die kleinste nocht nicht benutzte Nummer ist
directory_to_save_general = os.getcwd()
i = 0
while "Data_" + str(i) in os.listdir(directory_to_save_general):
    i += 1
subdirectory_name = "Data_" + str(i)
directory_to_save_this_time = directory_to_save_general + "\\" + subdirectory_name
os.mkdir(directory_to_save_this_time)

# Logge die Anfangszeit des Skriptes
file_path_timestamp = directory_to_save_this_time + "\\" + "creation_Timestamp" + '.csv'
f_timestamp = open(file_path_timestamp, 'w')
f_timestamp.write(str(datetime.now()))
f_timestamp.close()


# Hilfs Methoden

def logMessage(zeitstempel, port, art, nachricht):
    zuSchreiben = seperatorDesCSVZumSpeichern.join((port, art, nachricht))
    write_to_file_path = directory_to_save_this_time + "\\" + "Log.csv"
    f = open(write_to_file_path, 'a')
    f.write("\n" + zeitstempel + ", " + ": " + zuSchreiben)
    f.close()


def createCSVFile(name, port, headerAlsTeile):
    header = seperatorDesCSVZumSpeichern.join(headerAlsTeile)
    file_path = directory_to_save_this_time + "\\" + name + '.csv'
    f = open(file_path, 'w')
    f.write(header)
    f.close()
    logMessage("Vor Zeitabgleich", "File Created", port, "Datei " + name + ".csv wurde mit " + str(headerAlsTeile) + " erstellt.")


def writeToCSVFile(name, zeitstempel, port, zuSchreibenAlsTeile):
    zuSchreiben = seperatorDesCSVZumSpeichern.join([zeitstempel] + zuSchreibenAlsTeile)
    write_to_file_path = directory_to_save_this_time + "\\" + name + ".csv"
    f = open(write_to_file_path, 'a')
    f.write("\n" + zuSchreiben)
    f.close()
    logMessage(zeitstempel, port, " Data Added", name + ".csv got data: " + str(zuSchreibenAlsTeile))


def statusPrint(zeitstempel, port, nachricht):
    print(str(zeitstempel) + ", " + port + ", Status: " + nachricht)
    logMessage(zeitstempel, port, "Status Message sent", "Port " + port + "sent: " + str(nachricht))



def errorPrint(zeitstempel, port, nachricht):
    print(str(zeitstempel) + ", " + port + ", ERROR: " + nachricht)
    logMessage(zeitstempel, port, "Error Message sent", "Port " + port + " sent: " + str(nachricht))



def addSensor(nameDesSensors, Spalten: list[str], port = "---"):
    createCSVFile(nameDesSensors, port,  ["zeitstempel"] + Spalten)


# 2. Vorbereitung: Dateien für Logs Nachrichten und Sensor Daten erstellen

createCSVFile("Log", "---", ("port", "Art der Nachricht", "Nachricht"))

for sensor,values in Sensoren_Informationen.items():
    addSensor(sensor, values)


# Methode um Arduino Nachrichten verarbeiten

def nachrichtAusarbeiten(port, nachricht: str):
    aufteilung = [teil.strip() for teil in nachricht.strip().split(sep=seperatorDerNachrichtenTeileVomArduino)]
    if len(aufteilung) < 2:
        # Nachricht Format ist Falsch
        errorPrint(str(datetime.now()), port, "Nachricht Format ist Falsch (len(aufteilung) < 2): " + aufteilung)
        return
    
    zeitstempel = str(datetime.now())
    match aufteilung[0].strip():
        case "Value":
            if seperatorDesCSVVomArduino == seperatorDerNachrichtenTeileVomArduino:
                werte = [wert.strip() for wert in aufteilung[2:]]
            else:
                werte = [wert.strip() for wert in aufteilung[2].split(sep=seperatorDesCSVVomArduino)]
            writeToCSVFile(aufteilung[1].strip(), zeitstempel, port, werte)
        case "Status":
            match aufteilung[1].strip():
                case "Setup_Fertig":
                    statusPrint(zeitstempel, port, "Setup_Fertig")
                case "Print":
                    statusPrint(zeitstempel, port, aufteilung[2].strip())
        case "Error":
            match aufteilung[1].strip():
                case "Print":
                    errorPrint(zeitstempel, port, aufteilung[2].strip())


# Schleife fürs verarbeiten der Nachrichten

while True:
    # Alle 0,5 Sekunden
    time.sleep(0.5) 
    # In der Momentanen Implementierung kann der Zeitstempel bis zu der länge der Schlafezeit falsch sein
    # Demnach sollt die Schlafzeit klein bleiben oder, beim erhöhen der Schlafzeit, 
    # die Implementierung des Zeistempels verändert werden 
    # (z.B. RTC im Arduino sollte diese zur Nachricht hinzufügen)

    for serial in serials:
        # Für jede Verbindung/jeden Arduino
        while serial[1].in_waiting > 1: 
            #TODO: While Bedingung verbessern? Diese stelle könnte fehler verursachen, Wenn nur ein Teil der Nachricht ankam

            
            # Solange die Verbindung nicht leer ist
            try:
                # Versuche eine Zeile/Nachricht zu lesen, dekodieren und zu verarbeiten
                line = serial[1].readline()
                line = line.decode("utf-8")
                       
                nachrichtAusarbeiten(serial[0], line)
            except:
                # Sonst schreibe eine Error Nachricht
                errorPrint(str(datetime.now()), "---", serial[0], "Ungültige Nachricht: "+ line)


