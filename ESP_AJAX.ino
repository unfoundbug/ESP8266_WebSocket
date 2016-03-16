/*
FSWebServer - Example WebServer with SPIFFS backend for esp8266
Copyright (c) 2015 Hristo Gochkov. All rights reserved.
This file is part of the ESP8266WebServer library for Arduino environment.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
or you can upload the contents of a folder if you CD in that folder and run the following command:
for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done

access the sample web page at http://esp8266fs.local
edit the page by going to http://esp8266fs.local/edit
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, 14, NEO_GRB + NEO_KHZ400);

#define DBG_OUTPUT_PORT Serial

const char* ssid = "AjaxTest";
const char* password = "AJTest";
const char* host = "esp8266fs";
#define LED_BUILTIN BUILTIN_LED
ESP8266WebServer server(80);
//holds the current upload
File fsUploadFile;

//format bytes
String formatBytes(size_t bytes) {
	if (bytes < 1024) {
		return String(bytes) + "B";
	}
	else if (bytes < (1024 * 1024)) {
		return String(bytes / 1024.0) + "KB";
	}
	else if (bytes < (1024 * 1024 * 1024)) {
		return String(bytes / 1024.0 / 1024.0) + "MB";
	}
	else {
		return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
	}
}

String getContentType(String filename) {
	if (server.hasArg("download")) return "application/octet-stream";
	else if (filename.endsWith(".htm")) return "text/html";
	else if (filename.endsWith(".html")) return "text/html";
	else if (filename.endsWith(".css")) return "text/css";
	else if (filename.endsWith(".js")) return "application/javascript";
	else if (filename.endsWith(".png")) return "image/png";
	else if (filename.endsWith(".gif")) return "image/gif";
	else if (filename.endsWith(".jpg")) return "image/jpeg";
	else if (filename.endsWith(".ico")) return "image/x-icon";
	else if (filename.endsWith(".xml")) return "text/xml";
	else if (filename.endsWith(".pdf")) return "application/x-pdf";
	else if (filename.endsWith(".zip")) return "application/x-zip";
	else if (filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}

bool handleFileRead(String path) {
	DBG_OUTPUT_PORT.println("handleFileRead: " + path);
	if (path.endsWith("/")) path += "index.htm";
	String contentType = getContentType(path);
	String pathWithGz = path + ".gz";
	if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
		if (SPIFFS.exists(pathWithGz))
			path += ".gz";
		File file = SPIFFS.open(path, "r");
		size_t sent = server.streamFile(file, contentType);
		file.close();
		return true;
	}
	return false;
}

void handleFileUpload() {
	if (server.uri() != "/edit") return;
	HTTPUpload& upload = server.upload();
	if (upload.status == UPLOAD_FILE_START) {
		String filename = upload.filename;
		if (!filename.startsWith("/")) filename = "/" + filename;
		DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
		fsUploadFile = SPIFFS.open(filename, "w");
		filename = String();
	}
	else if (upload.status == UPLOAD_FILE_WRITE) {
		//DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
		if (fsUploadFile)
			fsUploadFile.write(upload.buf, upload.currentSize);
	}
	else if (upload.status == UPLOAD_FILE_END) {
		if (fsUploadFile)
			fsUploadFile.close();
		DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
	}
}

void handleFileDelete() {
	if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
	String path = server.arg(0);
	DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
	if (path == "/")
		return server.send(500, "text/plain", "BAD PATH");
	if (!SPIFFS.exists(path))
		return server.send(404, "text/plain", "FileNotFound");
	SPIFFS.remove(path);
	server.send(200, "text/plain", "");
	path = String();
}

void handleFileCreate() {
	if (server.args() == 0)
		return server.send(500, "text/plain", "BAD ARGS");
	String path = server.arg(0);
	DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
	if (path == "/")
		return server.send(500, "text/plain", "BAD PATH");
	if (SPIFFS.exists(path))
		return server.send(500, "text/plain", "FILE EXISTS");
	File file = SPIFFS.open(path, "w");
	if (file)
		file.close();
	else
		return server.send(500, "text/plain", "CREATE FAILED");
	server.send(200, "text/plain", "");
	path = String();
}

void handleFileList() {
	if (!server.hasArg("dir")) { server.send(500, "text/plain", "BAD ARGS"); return; }

	String path = server.arg("dir");
	DBG_OUTPUT_PORT.println("handleFileList: " + path);
	Dir dir = SPIFFS.openDir(path);
	path = String();

	String output = "[";
	while (dir.next()) {
		File entry = dir.openFile("r");
		if (output != "[") output += ',';
		bool isDir = false;
		output += "{\"type\":\"";
		output += (isDir) ? "dir" : "file";
		output += "\",\"name\":\"";
		output += String(entry.name()).substring(1);
		output += "\"}";
		entry.close();
	}

	output += "]";
	server.send(200, "text/json", output);
}
void colorWipe(uint32_t c) {
	for (uint16_t i = 0; i<strip.numPixels(); i++) {
		strip.setPixelColor(i, c);
	}
	strip.show();
}
void handleRGBSet()
{
	byte r = 0, g = 0, b = 0;
	r = atoi( server.arg(0).c_str() );
	g = atoi(server.arg(1).c_str());
	b = atoi(server.arg(2).c_str());
	DBG_OUTPUT_PORT.printf("RGB Request %s %s %s\n", server.arg(0).c_str(), server.arg(1).c_str(), server.arg(2).c_str());
	colorWipe(strip.Color(r, g, b));
}


byte byState = HIGH;
void addJSONHandlers()
{
	//get heap status, analog input value and all GPIO statuses in one json call
	server.on("/all", HTTP_GET, []() {
		String json = "{";
		json += "\"heap\":" + String(ESP.getFreeHeap());
		json += ", \"analog\":" + String(analogRead(A0));
		json += ", \"gpio\":" + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
		json += ", \"millis\":" + String(millis());
		json += "}";
		server.send(200, "text/json", json);
		
		json = String();
	});
	
}

void handleMotorSet()
{/*
	byte motL = 127, motR = 127;
	int iFindIter = 0;
	for (iFindIter = 0; iFindIter < server.args(); ++iFindIter)
	{
		String strArgName = server.argName(iFindIter);
		if (strArgName.c_str()[0] == 'm'
			&&
			strArgName.c_str()[1] == 'o'
			&&
			strArgName.c_str()[2] == 't'
			)
		{
			if (strArgName.c_str()[3] == 'L')
				motL = atoi(server.arg(iFindIter).c_str());
			if (strArgName.c_str()[3] == 'R')
				motR = atoi(server.arg(iFindIter).c_str());
		}
	}
	SDalekMotorPacket sPacket;
	sPacket.byPacketID = 0;
	sPacket.byDeviceID = 0;
	sPacket.byPacketDataX = 0;
	sPacket.byPacketDataY = 0;
	sPacket.byPacketDataZ = 0;
	AddCRC(&sPacket);
	Serial1.write((uint8_t*)&sPacket, 9);
	DBG_OUTPUT_PORT.printf("Mot request handled %02X %02X\n", motL, motR);*/
}
void addHandlers()
{
	//SERVER INIT
	//list directory
	server.on("/list", HTTP_GET, handleFileList);
	//load editor
	server.on("/edit", HTTP_GET, []() {
		if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
	});
	//create file
	server.on("/edit", HTTP_PUT, handleFileCreate);
	//delete file
	server.on("/edit", HTTP_DELETE, handleFileDelete);
	//first callback is called after the request has ended with all parsed arguments
	//second callback handles file uploads at that location
	server.on("/edit", HTTP_POST, []() { server.send(200, "text/plain", ""); }, handleFileUpload);

	server.on("/RGB", HTTP_GET, handleRGBSet);
	//called when the url is not defined here
	//use it to load content from SPIFFS
	server.onNotFound([]() {
		if (!handleFileRead(server.uri()))
			server.send(404, "text/plain", "FileNotFound");
	});

	addJSONHandlers();
}
// Fill the dots one after the other with a color

void OnTimer1()
{
	digitalWrite(LED_BUILTIN, byState);
	byState = 1 - byState;
}
void setup(void) {
	DBG_OUTPUT_PORT.begin(115200);
	DBG_OUTPUT_PORT.print("\n");
	DBG_OUTPUT_PORT.setDebugOutput(true);
	pinMode(LED_BUILTIN, OUTPUT);
	colorWipe(strip.Color(150, 0, 0));
	SPIFFS.begin();
	{
		Dir dir = SPIFFS.openDir("/");
		while (dir.next()) {
			String fileName = dir.fileName();
			size_t fileSize = dir.fileSize();
			DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
		}
		DBG_OUTPUT_PORT.printf("\n");
	}
	colorWipe(strip.Color(125, 25, 0));
	/*//timer dividers
	#define TIM_DIV1 	0 //80MHz (80 ticks/us - 104857.588 us max)
	#define TIM_DIV16	1 //5MHz (5 ticks/us - 1677721.4 us max)
	#define TIM_DIV265	3 //312.5Khz (1 tick = 3.2us - 26843542.4 us max)*/
	timer1_write(388607); 
	timer1_attachInterrupt(OnTimer1);
	timer1_enable(TIM_DIV265, TIM_LEVEL, TIM_LOOP);
	//WIFI INIT

	WiFi.mode(WiFiMode_t::WIFI_OFF);
	DBG_OUTPUT_PORT.printf("starting %s\n", ssid);
	if (true)
	{
		WiFi.softAPdisconnect(true);
		
		DBG_OUTPUT_PORT.printf("pre %s\n", WiFi.SSID().c_str());
		WiFi.softAP(ssid, password);
		DBG_OUTPUT_PORT.printf("post %s\n", WiFi.SSID().c_str());
		WiFi.enableSTA(false);
		WiFi.enableAP(true);
		WiFi.mode(WiFiMode_t::WIFI_AP);
		
		DBG_OUTPUT_PORT.println("");
		DBG_OUTPUT_PORT.print("Connected! IP address: ");
		DBG_OUTPUT_PORT.println(WiFi.softAPIP());
	}
	else
	{
		
		WiFi.disconnect();
		WiFi.begin(ssid, password);
		WiFi.enableSTA(true);
		WiFi.enableAP(false);
		WiFi.mode(WiFiMode_t::WIFI_STA);
		while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		DBG_OUTPUT_PORT.print(".");
		}

		DBG_OUTPUT_PORT.println("");
		DBG_OUTPUT_PORT.print("Connected! IP address: ");
		DBG_OUTPUT_PORT.println(WiFi.localIP());
	}
	
	colorWipe(strip.Color(75, 75, 0));
	MDNS.begin(host);
	DBG_OUTPUT_PORT.print("Open http://");
	DBG_OUTPUT_PORT.print(host);
	DBG_OUTPUT_PORT.println(".local/edit to see the file browser");
	colorWipe(strip.Color(25, 125, 0));
	addHandlers();
	
	server.begin();
	DBG_OUTPUT_PORT.println("HTTP server started");
	colorWipe(strip.Color(0,0, 0));
}

void loop(void) {
	server.handleClient();
}