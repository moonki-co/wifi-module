#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

char ssid[20];
char pass[20];
char user[40];

String ssid_leido;
String pass_leido;
String user_leido;

int ssid_tamano = 0;
int pass_tamano = 0;
int user_tamano = 0;

String arregla_simbolos(String a) {
 a.replace("%C3%A1", "á");
 a.replace("%C3%A9", "é");
 a.replace("%C3%A", "i");
 a.replace("%C3%B3", "ó");
 a.replace("%C3%BA", "ú");
 a.replace("%21", "!");
 a.replace("%23", "#");
 a.replace("%24", "$");
 a.replace("%25", "%");
 a.replace("%26", "&");
 a.replace("%27", "/");
 a.replace("%28", "(");
 a.replace("%29", ")");
 a.replace("%3D", "=");
 a.replace("%3F", "?");
 a.replace("%27", "'");
 a.replace("%C2%BF", "¿");
 a.replace("%C2%A1", "¡");
 a.replace("%C3%B1", "ñ");
 a.replace("%C3%91", "Ñ");
 a.replace("+", " ");
 a.replace("%2B", "+");
 a.replace("%22", "\"");
 return a;
}

//*******  G R A B A R  EN LA  E E P R O M  ***********
void graba(int addr, String a) {
 int tamano = (a.length() + 1);
 Serial.print(tamano);

 char inchar[30];    //'30' Tamaño maximo del string
 a.toCharArray(inchar, tamano);
 
 EEPROM.write(addr, tamano);
 
 for (int i = 0; i < tamano; i++) {
  addr++;
  EEPROM.write(addr, inchar[i]);
 }
 
 EEPROM.commit();
}

//**** CONFIGURACION WIFI  *******
void wifi_conf() {
 int cuenta = 0;
 String getssid = server.arg("ssid"); //Recibimos los valores que envia por GET el formulario web
 String getpass = server.arg("pass");
 String getuser = server.arg("user");

 getssid = arregla_simbolos(getssid); //Reemplazamos los simbolos que aparecen cun UTF8 por el simbolo correcto
 getpass = arregla_simbolos(getpass);
 getuser = arregla_simbolos(getuser);

 ssid_tamano = getssid.length() + 1;  //Calculamos la cantidad de caracteres que tiene el ssid y la clave
 pass_tamano = getpass.length() + 1;
 user_tamano = getuser.length() + 1;

 getssid.toCharArray(ssid, ssid_tamano); //Transformamos el string en un char array ya que es lo que nos pide WIFI.begin()
 getpass.toCharArray(pass, pass_tamano);
 getuser.toCharArray(user, user_tamano);

 Serial.println(ssid);     //para depuracion
 Serial.println(pass);
 Serial.println(user);

 while (WiFi.status() != WL_CONNECTED)
 {
  delay(500);
  Serial.print(".");
  cuenta++;
  if (cuenta > 20) {
   graba(70, "false");
   server.send(200, "application/json", "{\"status\": false, \"message\": \"No se pudo realizar la conexión\"}");
   return;
  }
 }
 Serial.print(WiFi.localIP());
 graba(70, "true");
 graba(1, getssid);
 graba(30, getpass);
 graba(80, getuser);

 server.send(200, "application/json", "{\"status\": true, \"message\": \"Conexión exitosa\"}");
}

//*******  L E E R   EN LA  E E P R O M    **************
String lee(int addr) {
 String nuevoString;
 int valor;
 int tamano = EEPROM.read(addr);

 for (int i = 0;i < tamano; i++) {
  addr++;
  valor = EEPROM.read(addr);
  nuevoString += (char)valor;
 }

 return nuevoString;
}

//*********  INTENTO DE CONEXION   *********************
void intento_conexion() {
 if (lee(70).equals("true")) {
  ssid_leido = lee(1);      //leemos ssid y password
  pass_leido = lee(30);

  Serial.println(ssid_leido);  //Para depuracion
  Serial.println(pass_leido);

  ssid_tamano = ssid_leido.length() + 1;  //Calculamos la cantidad de caracteres que tiene el ssid y la clave
  pass_tamano = pass_leido.length() + 1;

  ssid_leido.toCharArray(ssid, ssid_tamano); //Transf. el String en un char array ya que es lo que nos pide WiFi.begin()
  pass_leido.toCharArray(pass, pass_tamano);

  int cuenta = 0;
  WiFi.begin(ssid, pass);      //Intentamos conectar

  while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   cuenta++;
   if (cuenta > 20) {
    Serial.println("Fallo al conectar");
    return;
   }
  }
 }else {
  Serial.println("No existe configuracion almacenada");
  graba(1,"");
  graba(30,"");
  graba(70,"");
  graba(80,"");
  }

 if (WiFi.status() == WL_CONNECTED) {
  Serial.print("Conexion exitosa a: ");
  Serial.println(ssid);
  Serial.println(WiFi.localIP());
 }else{
  Serial.println("El estatus del wifi es: No conectado");
  Serial.println(WiFi.status());
  Serial.println(WiFi.localIP());
  }
}

//*****  S E T U P  **************
void setup() {
 Serial.begin(115200);
 EEPROM.begin(4096);
 pinMode(0,INPUT);

 WiFi.softAP("Moonki");      //Nombre que se mostrara en las redes wifi

 server.on("/_health", []() {
  server.send(200, "application/json", "{\"status\": true, \"server\": \"moonki\"}");
 });

 server.on("/_data", []() {
  ssid_leido = lee(1);  
  pass_leido = lee(30);
  user_leido = lee(80);
  Serial.println(ssid_leido);
  Serial.println(pass_leido);
  Serial.println(user_leido);

  server.send(200, "application/json", "{\"ssid\": \"" + ssid_leido + "\", \"pass\": \"" + pass_leido + "\", \"user\": \"" + user_leido + "\"}");
 });


 server.on("/config", wifi_conf);

 server.begin();
 Serial.println("Webserver iniciado...");
 Serial.println(lee(70));
 Serial.println(lee(1));
 Serial.println(lee(30));
 Serial.println(lee(80));
 
 intento_conexion();
}

//*****   L O O P   **************
void loop() {
  if(digitalRead(0)==HIGH){
 Serial.println(user_leido);
  }
 server.handleClient();
 delay(2000);
 

 
 }
