/**
 * @author Alexander Entinger, MSc / LXRobotics
 * @brief LXRobotics Wake-On-Lan-Server with Arduino Ethernet
 * @file lxrobotics_arduino_wol_server.ino
 */
 
/* CREDITS:
 * webserver part based on https://github.com/sirleech/Webduino
 * wake-on-lan-code by https://github.com/mikispag/arduino-WakeOnLan/blob/master/arduino-WakeOnLan.ino
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "WebServer.h"

// target max address
static byte target_mac[] = { 0xbc, 0x5f, 0xf4, 0xdd, 0x0e, 0x1d }; 

// MAC- and IP-Address
static byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0x95 };
static byte ip[] = { 192, 168, 1, 3 };
static byte my_dns[] = { 192, 168, 1, 1 };
static byte gateway[] = { 192, 168, 1, 1 };
static byte subnet[] = { 255, 255, 255, 224 };
static byte broadcast[] = { 192, 168, 1, 31 };

// instantiate a udp object
EthernetUDP my_udp;

/**
 * @brief sends the wol package - from https://github.com/mikispag/arduino-WakeOnLan/blob/master/arduino-WakeOnLan.ino
 */
void send_wol_packet(byte target_mac[]) 
{
    byte preamble[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    my_udp.beginPacket(broadcast, 7);
    my_udp.write(preamble, 6);
    
    for (int i=0; i<16; i++)
    {
      my_udp.write(target_mac, 6);
    }
    
    my_udp.endPacket();
}

// This creates an instance of the webserver.  By specifying a prefix of "/", all pages will be at the root of the server.
#define PREFIX "/"
WebServer webserver(PREFIX, 80);

// no-cost stream operator as described at http://sundial.org/arduino/?page_id=119
template<class T>
inline Print &operator <<(Print &obj, T arg)
{ obj.print(arg); return obj; }

/**
 * @brief this page represents the start page
 */
void index_cmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  if (type == WebServer::POST)
  {
    boolean repeat;
    char name[20], value[20];
    
    boolean user_name_correct = false;
    boolean pass_word_correct = false;
    
    do
    {
      repeat = server.readPOSTparam(name, 20, value, 20);
      
      // check for correct user
      if ((strcmp(name,"user") == 0) && (strcmp(value,"wol-server-user") == 0))
      {
         user_name_correct = true; 
      }
      
      // check for correct password
      if ((strcmp(name,"pass") == 0) && (strcmp(value,"wol-server-user") == 0))
      {
         pass_word_correct = true; 
      }
      
    } while (repeat);
    
    // if both user and password are correct, trigger the wol packet
    if(user_name_correct && pass_word_correct)
    {
       Serial.println("Sending WOL packet");
       send_wol_packet(target_mac);
    }

   // after procesing the POST data, tell the web browser to reload the page using a GET method. 
   server.httpSeeOther(PREFIX);
   return;
  }
  
  // send standard everything allright answer
  server.httpSuccess();

  // if not just a single head request deliver content here
  if (type == WebServer::GET)
  {
    server << "<html><body>";
    server << "<form action='/' method='post'>";
    server << "<table>";
    server << "<tr><td>Benutzername:</td><td><input type=\"text\" name=\"user\" size=\"20\" maxlength=\"20\">";
    server << "<tr><td>Passwort:</td><td><input type=\"password\" name=\"pass\" size=\"20\" maxlength=\"20\">";
    server << "</table>";
    server << "<input type='submit' value='Send WOL Packet'/></form>";
    server << "</body></html>";
  }
}

/**
 * @brief setting up the device
 */
void setup()
{
  // start the serial interface up
  Serial.begin(9600);
  
  // initialize the ethernet device not using DHCP:
  Ethernet.begin(mac, ip, my_dns, gateway, subnet); 

  // print the ip address
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());
  
  // startup udp
  my_udp.begin(7);
  
  // setup our default command that will be run when the user accesses the root page on the server
  webserver.setDefaultCommand(&index_cmd);

  // start the webserver
  webserver.begin();
}

/**
 * @brief the main processing loop
 */
void loop()
{
  char buff[64];
  int len = 64;

  /* process incoming connections one at a time forever */
  webserver.processConnection(buff, &len);
}
