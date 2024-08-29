/*
Nachrichten an den Computer/ Serial.print Nachrichten:

 Nachrichten an den Computer werden mit serialPrint() gesendet.
 Dies sollte nur über hier gemachte structs und Methoden gemacht werden, um Einheitlichkeit zu gewährleisten.

Nachrichten sind in einzelne Teile Unterteilt, welche mit "|" separiert sind und können diese Formen habe:
 "Art der Nachricht|Unterart", z.B.: "Zeit seit 0|Status|Verbindung_Erfolgreich"
 "Art der Nachricht|Unterart|Text", z.B.: "Zeit seit 0|Status|Print|Nachricht"
 "Art der Nachricht|name_des_Sensors|csv", z.B.: "Zeit seit 0|Value|name_des_Sensors|Wert_Spalte_1, Wert_Spalte_2, Wert_Spalte_3, ..."

CSV benutz im Moment auch den seperator |. Der Python Code muss angepasst werden, bevor anderen CSV Seperatoren benutzt werden können.

Es gibt 3 Arten von Serial.print Nachrichten:

Value:
    Nachricht hat den Namen des Sensors (name der csv datei) gefolgt von den Werten des Sensors in Gleicher Reihenfolge wie im Python Code.

    Form:
        Serial.println(Value|name_des_Sensors|Wert_Spalte_1, Wert_Spalte_2, Wert_Spalte_3, ...)
    Beispiel:
        Serial.println(Value|GPS| 49.784515, 9.975503, 322.10, 0.00, 110, 14)
    
    Diese Nachrichten werden vom struct Sensor_UB gesendet.
        Dafür wird vor dem Setup für jeden Sensor ein Sensor_UB objekt erstellt werden, mit der Linie:
            const Sensor_UB VariablenName(SensorName, AnzahlDerSpalten)
                wobei SensorName der gleiche Name als String wie im Python Code ist
            z.B. const Sensor_UB GPS("GPS", 6);

        Zum Senden wird im Sensor Code im loop() (z.B. GPSstuff()) die sendValue() Funktion, des passenden structs, benutzt, mit der linie:
            VariablenName.sendValues(WerteArray)
                wobei WerteArray eine Konstante String Array der Werte mit länge AnzahlDerSpalten ist.
            oder alternativ, wenn AnzahlDerSpalten = 1 ist:
            Luftfeuchte_DHT.sendValues(&humidity);
                wobei humidity ein konstanter String ist
            z.B.:
                String gps_Values[6];
                ...Daten einfügen...
                const String gps_Values_Array[] = { gps_Values[0], gps_Values[1], gps_Values[2], gps_Values[3], gps_Values[4], gps_Values[5] };
                GPS.sendValues(gps_Values_Array);
                bzw.
                const String humidity = String(dht.readHumidity());
                Luftfeuchte_DHT.sendValues(&humidity);

    

Status:
    Spezielle Nachrichten die keine Fehler darstellen.

    Diese Status Nachrichten sind implementiert.
        Setup_Fertig: Wird am Ende des Setups benutzt, also nachdem die Sensoren Beschreibungen gesendet wurden.
            Form:
                Serial.println(Status|Setup_Fertig)
            Beispiel:
                Serial.println("Status|Setup_Fertig")

        Print: Druckt die mitgesendete Nachricht aus.
            Form:
                Serial.println(Status|Print|Nachricht)
            Beispiel:
                Serial.println(Status|Print|Setup fertig) -> Druckt "Setup fertig"

    Wenn andere Funktionen gewollt sind müssen diese Implementiert und dokumentiert werden.


Error:
    Spezielle Nachrichten die Fehler darstellen.

    Diese Error Nachrichten sind implementiert.
        Print: Druckt die mitgesendete Nachricht aus.
            Form:
                Serial.println(Error|Print|Nachricht)
            Beispiel:
                Serial.println(Error|Print|Fehler bei ...) -> Druckt "Fehler bei ..."

    Wenn andere Funktionen gewollt sind müssen diese Implementiert und dokumentiert werden.

Beide werden mit dem Nachricht Struct gesendent, wobei die Arten und Unterarten in den enums Nachricht_Art und Nachricht_Unterart.
Dabei wird zuerst ein Struct Objekt für jede Möglich Nachricht instanziiert mit der Linie:
    Nachricht<Nachricht_Art::Art, Nachricht_Unterart::Unterart> VariablenName
    z.B. const Nachricht<Nachricht_Art::STATUS, Nachricht_Unterart::PRINT> statusPrint;
Nachrichten werden, dann mit der send() Funktion gesendet, entweder ohne Parameter oder mit einen String der gedruckt werden soll:
    VariablenName.send(); 
    VariablenName.send(StringZuDrucken);
    z.B.:
        statusSetupFertig.send();
        statusPrint.send("GPS begonnen");

Um neue Nachrichten zu erlauben muss dessen Art und Unterart komKombination in der Bedingung von static_assert im Struct erlaubt sein.

*/

// Seperator für CSV
const String seperator = "|";

// Struct für die Sensoren
struct Sensor_UB {
    const String name;
    size_t numColumns;

    Sensor_UB(const String& sensorName, size_t numCols)
        : name(sensorName), numColumns(numCols) {}

    // Sende die Value Nachricht
    void sendValues(const String* values) const {
        String nachricht = "Value|" + name + "|";
        for (size_t i = 0; i < numColumns; ++i) {
            nachricht += values[i];
            if (i < numColumns - 1) {
                nachricht += seperator;
            }
        }
        Serial.println(nachricht);
    }
};
enum Nachricht_Art {
    STATUS,
    ERROR,
};

enum Nachricht_Unterart {
    SETUP_FERTIG,
    PRINT,
};

String enumToStringArt(Nachricht_Art art) {
    switch (art) {
        case Nachricht_Art::STATUS:
            return "Status";
        case Nachricht_Art::ERROR:
            return "Error";
        default:
            return "Unknown";
    }
}

String enumToStringUnterart(Nachricht_Unterart unterart) {
    switch (unterart) {
        case Nachricht_Unterart::SETUP_FERTIG:
            return "Setup_Fertig";
        case Nachricht_Unterart::PRINT:
            return "Print";
        default:
            return "Unknown";
    }
}


// Struct für Status und Error Nachrichten
template<Nachricht_Art Art, Nachricht_Unterart Unterart>
struct Nachricht {
    static_assert(
        (Art == Nachricht_Art::STATUS &&
            (Unterart == Nachricht_Unterart::SETUP_FERTIG ||
             Unterart == Nachricht_Unterart::PRINT)
        ) ||
        (Art == Nachricht_Art::ERROR && Unterart == Nachricht_Unterart::PRINT),
        "Eine Nachricht mit Art und Unterart Kombination ist nicht definiert"
    );

    String nachricht = enumToStringArt(Art) + "|" + enumToStringUnterart(Unterart);



    void send() {
        Serial.println(nachricht);
    };

    void send(const String zusatz) {
        Serial.println(nachricht + "|" + zusatz);
    };
};
