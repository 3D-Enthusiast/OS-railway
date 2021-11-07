/* Copyright Olle Sköld 2017
 * 
 * Wifi accesspoitn using NodeMCU and ESP8266.
 * 
 * This software is provided free and you may use, change and distribute as you like.
 *
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include "TimerObject.h"

#define forward 0
#define backward 1
/* Set these to your desired credentials. */
const char *ssid = "OSRHector";
const char *password = "osrailway";


bool highBeamActive = false;
bool motionActive = false;
bool motionDirection = forward;
int target_speed;
int actual_speed;
int acceleration_step = 5;

int LED_BL2 = 15; // Red light on side 2 GPIO15
int LED_BL1 = 13; // Red light on side 1 GPIO13
int LED_HB2 = 14; // High beam side 2 GPIO12
int LED_HB1 = 12; // High beam side 1 GPIO14

int LED_HDL = 2;  //PWM for headlights controlled by motor board

int motor_AIN1 = 5; //GPIO5
int motor_AIN2 = 4; //GPIO4
int motor_PWM = 0; //GPIO0

TimerObject *timer1 = new TimerObject(10);

ESP8266WebServer server(80);

/************************************************************************** 
 *  
 *  Server IP address is: http://192.168.4.1
 *  
 *  When you have connected your device to the wifi network, open the browser and enter the IP address above.
 *  
 */
void handleRoot() {
  /************************************************************************ 
  * This is the web page sent to the connected device. There are lots of memory available so this page can be made much more complex. 
  * It uses Ajax to send commands which means the page doesn't need to be reloaded each time a button is pressed. In this way, 
  * the page can be made much more graphically complex without suffering from long reloading delays.
  */
  String html ="<html> <header> <style type=\"text/css\"> body { background: #595B82; } #header { height: 30px; margin-left: 2px; margin-right: 2px; background: #d5dfe2; border-top: 1px solid #FFF; border-left: 1px solid #FFF; border-right: 1px solid #333; border-bottom: 1px solid #333; border-radius: 5px; font-family: \"arial-verdana\", arial; padding-top: 10px; padding-left: 20px; } #speed_setting { float: right; margin-right: 10px; } #control { height: 200px; margin-top: 4px; margin-left: 2px; margin-right: 2px; background: #d5dfe2; border-top: 1px solid #FFF; border-left: 1px solid #FFF; border-right: 1px solid #333; border-bottom: 1px solid #333; border-radius: 5px; font-family: \"arial-verdana\", arial; } .button { border-top: 1px solid #FFF; border-left: 1px solid #FFF; border-right: 1px solid #333; border-bottom: 1px solid #333; border-radius: 5px; margin: 5px; text-align: center; float: left; padding-top: 20px; height: 50px; background: #FFF} .long { width: 30%; } .short { width: 100px; } #message_box { float: left; margin-bottom: 0px; width: 100%; } #speed_slider {width: 300px;}</style> <script type=\"text/javascript\"> function updateSpeed(speed) { sendData(\"updateSpeed?speed=\"+speed); } function runForward(){ var speed = document.getElementById(\"speed_slider\").value; sendData(\"run?dir=forward&speed=\"+speed); } function runBackward(){ var speed = document.getElementById(\"speed_slider\").value; sendData(\"run?dir=backward&speed=\"+speed); } function sendData(uri){ var messageDiv = document.getElementById(\"message_box\"); messageDiv.innerHTML = \"192.168.4.1/\"+uri; var xhr = new XMLHttpRequest(); xhr.open('GET', 'http://192.168.4.1/'+uri); xhr.withCredentials = true; xhr.setRequestHeader('Content-Type', 'text/plain'); xhr.send(); } </script> </header> <body> <div id=\"header\"> OS-Railway Wifi Hectorrail 141 <div id=\"speed_setting\"> Speed <input id=\"speed_slider\" type=\"range\" min=\"0\" max=\"1023\" step=\"10\" value=\"512\" onchange=\"updateSpeed(this.value)\"> </div> </div> <div id=\"control\"> <div id=\"button_backward\" class=\"button long\" onclick=\"runBackward()\"> Run backward </div> <div id=\"button_stop\" class=\"button long\" onclick=\"updateSpeed(0)\"> Stop </div> <div id=\"button_forward\" class=\"button long\" onclick=\"runForward()\"> Run forward </div> <div id=\"button_light_off\" class=\"button long\" onclick=\"sendData('lightoff')\"> Headlight off </div> <div id=\"button_light_on\" class=\"button long\" onclick=\"sendData('lighton')\"> Headlight on </div> <div id=\"button_emergency\" class=\"button long\" onclick=\"sendData('stop')\"> Emergency stop </div> <div id=\"message_box\"> </div> </div> </body> </html>";
  server.send(200, "text/html", html);
}

void motionControl(){
  if(motionActive){
    if(actual_speed < target_speed){
      actual_speed = actual_speed + acceleration_step;
    } else if(actual_speed > target_speed){
      actual_speed = actual_speed - acceleration_step;
    }    
    if(actual_speed > 1023){
      actual_speed = 1023;
    }
    if(actual_speed < 0){
      actual_speed = 0;
    }
    analogWrite(motor_PWM, actual_speed);
  }  else {
    analogWrite(motor_PWM, 0);
  }
}

/**************************************************
 *  Motor operation
 */

void motorOperation(){

  motionActive = true;  
  String move_dir = server.arg("dir");
  String motion_speed = server.arg("speed");
  target_speed = motion_speed.toInt();  

  //Execute change of direction, but only if actual_speed is below 100, otherwise set target speed to 0 and initiate a slowdown.  
  if(move_dir == "backward"){
    if(motionDirection != backward && actual_speed > 100){
      target_speed = 0;
    } else {
      motionDirection = backward;
    }
  } else {
    if(motionDirection != forward && actual_speed > 100){
      target_speed = 0;
    } else {
      motionDirection = forward;
    }
  }

  digitalWrite(motor_AIN1, motionDirection);
  digitalWrite(motor_AIN2, !motionDirection);
  digitalWrite(LED_BL2, motionDirection);
  digitalWrite(LED_BL1, !motionDirection);
    
  if(highBeamActive){
    digitalWrite(LED_HB2, motionDirection);
    digitalWrite(LED_HB1, !motionDirection);
  } else {
    digitalWrite(LED_HB2, 0);
    digitalWrite(LED_HB1, 0);
  }

  
}

void updateSpeed(){
  String motion_speed = server.arg("speed");
  target_speed = motion_speed.toInt();  
}

void switchLightOn(){
    highBeamActive = true;
    digitalWrite(LED_HB2, motionDirection);
    digitalWrite(LED_HB1, !motionDirection);
}

void switchLightOff(){
    highBeamActive = false;
    digitalWrite(LED_HB2, 0);
    digitalWrite(LED_HB1, 0);  
}

void motionStop(){
  motionActive = false;
  analogWrite(motor_PWM, 0);    
}

/*********************************************************************
 * SETUP
 */

void setup() {
	delay(1000);

  pinMode(LED_BL2, OUTPUT);
  pinMode(LED_BL1, OUTPUT);
  pinMode(LED_HB2, OUTPUT);
  pinMode(LED_HB1, OUTPUT);
  
  pinMode(LED_HDL, OUTPUT);
  
  pinMode(motor_AIN1, OUTPUT);
  pinMode(motor_AIN2, OUTPUT);
  pinMode(motor_PWM, OUTPUT);

  
  digitalWrite(LED_BL2, 0);
  digitalWrite(LED_BL1, 0);
  digitalWrite(LED_HB2, 0);
  digitalWrite(LED_HB1, 0);

  digitalWrite(motor_AIN1, 0);
  digitalWrite(motor_AIN2, 1);

  analogWrite(motor_PWM, 0);
  analogWrite(LED_HDL, 1023);

  timer1->setOnTimer(&motionControl);
  timer1->Start();
  
	WiFi.softAP(ssid, password);

	IPAddress myIP = WiFi.softAPIP();

 /****************************************************************************
  * Here you find the functions that are called when the browser sends commands. 
  */
	server.on("/", handleRoot);
  server.on("/run", motorOperation);
  server.on("/stop", motionStop);
  server.on("/lighton", switchLightOn);
  server.on("/lightoff", switchLightOff);
  server.on("/updateSpeed", updateSpeed);
	server.begin();
}

void loop() {
	server.handleClient();
  timer1->Update();
}
