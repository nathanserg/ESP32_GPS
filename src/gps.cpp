#include <Arduino.h>
#include <TinyGPS++.h>
#include <math.h>
#include <stdint.h>
#include "gps.h"
#include "graphics.h"
#include "conf.h"

TinyGPSPlus gps;
extern HardwareSerial serialGPS;
extern Coord gps_coord;

#define DEG2RAD(a) ((a) / (180.0 / M_PI))
#define RAD2DEG(a) ((a) * (180.0 / M_PI))
#define EARTH_RADIUS 6378137
double lat2y(double lat) { return log(tan( DEG2RAD(lat) / 2 + M_PI/4 )) * EARTH_RADIUS; }
double lon2x(double lon) { return DEG2RAD(lon) * EARTH_RADIUS; }

String msg;
/// @brief Updates the current position in gps_coord from the GPS data 
void getPosition()
{
  const uint32_t readDelay = 200; // ms
  static uint32_t lastReadTime = 0;

  // check interval
  if(( millis() - lastReadTime) < readDelay) return;
  lastReadTime = millis();

  while( serialGPS.available() > 0){
    char c = serialGPS.read();
    gps.encode(c);
    
    // debug
    if( c == '\n'){
      LOGV("%s", msg.c_str());
      msg.clear();
    } else if( c != '\r'){
      msg += c;
    }    
  }
  if( gps.location.isValid()){
    gps_coord.isValid = true;

    gps_coord.lat = gps.location.lat();
    if( abs( gps_coord.prev_lat - gps_coord.lat) > 0.0001){
      LOGD("prev_lat:%f, lat:%f", gps_coord.prev_lat, gps_coord.lat);
      gps_coord.prev_lat = gps_coord.lat;
      gps_coord.isUpdated = true;
    }
    gps_coord.lng = gps.location.lng();
    if( abs( gps_coord.prev_lng - gps_coord.lng) > 0.0001){
      LOGD("prev_lng:%f, lng:%f", gps_coord.prev_lng, gps_coord.lng);
      gps_coord.prev_lng = gps_coord.lng;
      gps_coord.isUpdated = true;
    }
    gps_coord.satellites = static_cast<int16_t>(gps.satellites.value());
    if( gps_coord.prev_satellites != gps_coord.satellites){
      LOGD("prev_sats:%i, sats:%i", gps_coord.prev_satellites, gps_coord.satellites);
      gps_coord.prev_satellites = gps_coord.satellites;
      // gps_coord.isUpdated = true;  //TODO: update only header
    }
    gps_coord.altitude = static_cast<int16_t>(gps.altitude.meters());
    if( abs( gps_coord.prev_altitude - gps_coord.altitude) > 10){
      LOGD("prev_alt:%i, alt:%i", gps_coord.prev_altitude, gps_coord.altitude);
      gps_coord.prev_altitude = gps_coord.altitude;
      gps_coord.isUpdated = true;
    }
    gps_coord.direction = static_cast<int16_t>(gps.course.deg()); // degrees
    // gps_coord.hour = gps.time.hour();
    // gps_coord.minute = gps.time.minute();
    // gps_coord.second = gps.time.second();
    if( gps_coord.satellites >= 4 && !gps_coord.fixAcquired){
      gps_coord.fixAcquired = true;
      gps_coord.isUpdated = true;
      serialGPS.println("$PMTK225,8*23"); // set 'Alwayslocate' mode
      LOGD("Fix Acquired! Sats: %i", gps_coord.satellites);
    } else if( gps_coord.satellites <= 3 && gps_coord.fixAcquired){
      gps_coord.fixAcquired = false;
      gps_coord.isUpdated = true;
      serialGPS.println("$PMTK225,0*2B"); // set 'full on' mode
      LOGD("No fix! Sats: %i", gps_coord.satellites);
    }
  } else {
    gps_coord.isValid = false;
  }
}

Point32 Coord::getPoint32()
{
  return Point32( lon2x( lng), lat2y( lat));
}


// Serial.print("LAT=");  Serial.println(gps.location.lat(), 6);
// Serial.print("LONG="); Serial.println(gps.location.lng(), 6);
// Serial.print("ALT=");  Serial.println(gps.altitude.meters());
// Serial.print("day ");  Serial.println(gps.date.day());
// Serial.print("year ");  Serial.println(gps.date.year());
// Serial.print("hour ");  Serial.println(gps.time.hour());
// Serial.print("minute ");  Serial.println(gps.time.minute());
// Serial.print("satellites ");  Serial.println(gps.satellites.value());