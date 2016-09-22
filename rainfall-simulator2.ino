#include "RelayShield/RelayShield.h"
int RELAY1 = D3;
int RELAY2 = D4;
int RELAY3 = D5;
int RELAY4 = D6;

//NFDF API POST Syntax
//http://graphical.weather.gov/xml/SOAP_server/ndfdXMLclient.php?whichClient=NDFDgen&lat=38.99&lon=-77.01&listLatLon=&lat1=&lon1=&lat2=&lon2=&resolutionSub=&listLat1=&listLon1=&listLat2=&listLon2=&resolutionList=&endPoint1Lat=&endPoint1Lon=&endPoint2Lat=&endPoint2Lon=&listEndPoint1Lat=&listEndPoint1Lon=&listEndPoint2Lat=&listEndPoint2Lon=&zipCodeList=&listZipCodeList=&centerPointLat=&centerPointLon=&distanceLat=&distanceLon=&resolutionSquare=&listCenterPointLat=&listCenterPointLon=&listDistanceLat=&listDistanceLon=&listResolutionSquare=&citiesLevel=&listCitiesLevel=&sector=&gmlListLatLon=&featureType=&requestedTime=&startTime=&endTime=&compType=&propertyName=&product=time-series&begin=2004-01-01T00%3A00%3A00&end=2020-09-18T00%3A00%3A00&Unit=e&qpf=qpf&Submit=Submit

// called once on startup
void setup() {

    // Listen for the webhook response, and call gotWeatherData()
    Particle.subscribe("hook-response/Opti_QPF", gotWeatherData, MY_DEVICES);
        delay(1000);
        
    //Initilize the relay control pins as output
   pinMode(RELAY1, OUTPUT);
   pinMode(RELAY2, OUTPUT);
   pinMode(RELAY3, OUTPUT);
   pinMode(RELAY4, OUTPUT);
   // Initialize all relays to an OFF state
   digitalWrite(RELAY1, LOW);
   digitalWrite(RELAY2, LOW);
   digitalWrite(RELAY3, LOW);
   digitalWrite(RELAY4, LOW);    
        
}

void loop() {

    // Request the weather, but no more than once every 60

    // publish the event that will trigger Webhook
    Particle.publish("Opti_QPF");

    // and wait at least 60 seconds before doing it again
    delay(600000); //this delay should be the same as pumpcyclelen below
}

// This function will get called when weather data comes in
void gotWeatherData(const char *name, const char *data) {
    String str = String(data);
    String qpfStr = tryExtractString(str, "Amount</name>", "</value>");

    if (qpfStr != NULL) {
        
        //USER INPUTS AND CONSTANTS
        
        float demodia = 24; //Diameter of demo tank
        float volcoeff = 0.8; //Volumetric Runoff Coefficient for virtual watershed
        float pondarea = 1000; //Surface area of virtual BMP in sq ft (assumes vertical sidewalls)
        float watershedarea = 1; //Area of the watershed draining to the virtual BMP
        float pumpcyclelen = 600; //seconds
        float pumprate = 2; //pump rate when on in gpm
        
        //VALUE FROM WEBHOOK
        float qpfNum = qpfStr.toFloat(); // note qpfNum is the expected rain in the current 6 hour qpf window
        qpfNum = 3; //force qpf to be some number for testing - units are in per 6 hours

        //CONVERSIONS AND CALCULATIONS
        
        watershedarea = watershedarea * 43560; //unit conversion to sq.ft.
        float demoarea = ((demodia/2)*(demodia/2) * 3.14159) / 144; //area of demo unit tank
        float pumprategps = pumprate/60; //in gps
        float runoffrate = (((qpfNum/6)/12)*watershedarea*volcoeff)*7.48/60/60;
        float demounitrate = runoffrate * (demoarea/pondarea);
        float fracttime = demounitrate/pumprategps;
        float pumpintime = fracttime*pumpcyclelen; 
        
        //PUBLICATION
        
        String qpfNumStr = String(qpfNum);
        String pumpintimeStr = String(pumpintime);
        Particle.publish("qpfNumStr", qpfNumStr, PRIVATE);
        Particle.publish("pumpintime", pumpintimeStr, PRIVATE);
        Particle.publish("QPF", qpfStr, PRIVATE);
        
        //ACTIONS
        
        digitalWrite(RELAY1, HIGH);
        delay(pumpintime*1000); //note that this delay means that the actual total delay between pump cycle starts is not exactly pumpcyclelen. Ignore if pumpcyclelen is large compared with the actual delay.
        digitalWrite(RELAY1, LOW);
        
    }
}

// Returns any text found between a start and end string inside 'str'
String tryExtractString(String str, const char* start, const char* end)
{
    if (str == NULL) {
        return NULL;
    }

    int idx = str.indexOf(start);
    if (idx < 0) {
        return NULL;
    }

    int endIdx = str.indexOf(end);
    if (endIdx < 0) {
        return NULL;
    }

    return str.substring(idx + strlen(start) + 16, endIdx);
}
