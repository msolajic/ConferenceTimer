#include "Arduino.h"
#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h> 
#include <WiFiManager.h> 
#include <ESP8266mDNS.h>

// Global Wi-Fi client instance
WiFiClient client;

// Hardware configuration for MAX7219 LED matrix
#define NUM_MAX 2  // Number of 8x8 LED matrix segments modules connected in series

// Pin definitions for WEMOS D1 Mini hardware SPI / bit-banging
#define DIN_PIN D7 // Data Input pin
#define CS_PIN  D8 // Chip Select pin
#define CLK_PIN D5 // Clock pin

// Custom low-level driver headers for controlling the MAX7219 and character fonts
#include "max7219.h"
#include "font.h"

// Server instances initialization
ESP8266WebServer server(80);          // HTTP server running on standard port 80
WebSocketsServer webSocket = WebSocketsServer(81); // WebSocket server running on port 81 for real-time sync

// Core Timer state variables
int totalMinutes = 30;                 // Default initial timer duration
unsigned long previousMillis = 0;      // Tracks the last time (in ms) the countdown decremented
long remainingSeconds = totalMinutes * 60; // Total countdown time tracked in seconds (can go negative)
bool timerRunning = false;             // Flag indicating whether the countdown is active
unsigned long lastActivityTime = 0;    // Tracks the timestamp of the last user interaction for power saving
bool matrixOffTimeout = false;         // Flag indicating if the display is currently in low-power sleep mode

// LED Display rendering variables
int dots = 0;                          // Toggles between 0 and 1 to create the blinking colon effect
long dotTime = 0;                      // Tracks the last time the blinking colon state changed
int dx = 0;                            // Display offset X (used for shifting text/digits)
int dy = 0;                            // Display offset Y (used for shifting text/digits)
unsigned long flashTime = 0;           // Tracks the flashing frequency when the timer hits negative values
bool displayIsOn = true;               // Current power-on state of the MAX7219 matrix hardware

// Include HTML interface stored in flash memory (PROGMEM)
#include "html.h"

/**
 * HTTP GET Root handler. Serves the main responsive control panel webpage.
 */
void handleRoot() {
  server.send_P(200, "text/html", HTML_CODE);
}

void handleFullscreen() {
  server.send_P(200, "text/html", HTML_FULLSCREEN);
}

/**
 * HTTP GET Command handler. Processes AJAX control requests sent by the web UI dashboard.
 */
void handleCommand() {
  String action = server.arg("action");
  
  // User interacted with the web interface -> Reset inactivity timeout counter
  lastActivityTime = millis();
  if (matrixOffTimeout) {
    matrixOffTimeout = false;
    sendCmdAll(CMD_SHUTDOWN, 1); // Wake up the MAX7219 display from sleep mode
  }
  
  if (action == "start") {
    // Check if a specific time configuration was submitted along with the start command
    if (server.hasArg("minutes")) {
      int inputMinutes = server.arg("minutes").toInt();
      if (inputMinutes != totalMinutes) {
        totalMinutes = inputMinutes;
        remainingSeconds = totalMinutes * 60;
      }
    }
    // Loop back safety: If starting from 0 or negative overrun, reset to the configured duration
    if (remainingSeconds <= 0) {
      remainingSeconds = totalMinutes * 60;
    }
    timerRunning = true;
  } 
  else if (action == "pause") {
    timerRunning = false;
  } 
  else if (action == "reset") {
    timerRunning = false;
    
    // Dynamic value assignment: Accept new duration configuration during runtime reset
    if (server.hasArg("minutes")) {
      int inputMinutes = server.arg("minutes").toInt();
      totalMinutes = inputMinutes; 
    }
    
    remainingSeconds = totalMinutes * 60;
    dots = 1;         // Keep the separation colon solid on reset
    sendTimeToWeb();  // Broadcast updated time status immediately to all connected clients
  }
  
  server.send(200, "text/plain", "OK");
}

/**
 * Format time to MM:SS or -MM:SS string and broadcast it to all connected WebSocket clients.
 */
void sendTimeToWeb() {
  long absoluteSeconds = abs(remainingSeconds);
  int m = absoluteSeconds / 60;
  int s = absoluteSeconds % 60;
  char buffer[10];
  
  // Append a negative sign if the speaker has exceeded their allocated conference time
  if (remainingSeconds < 0) {
    sprintf(buffer, "-%02d:%02d", m, s);
  } else {
    sprintf(buffer, "%02d:%02d", m, s);
  }
  webSocket.broadcastTXT(buffer); // Pushes real-time update to all open web browser instances
}

/**
 * WebSocket event router callback. Triggered upon client connections or incoming frames.
 */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if(type == WStype_CONNECTED) {
    sendTimeToWeb(); // Send the current timer state instantly to a newly joined client
  }
}

/**
 * WiFiManager Callback routine. Executed if the ESP8266 fails to connect to known networks 
 * and fallback enters Captive Portal Configuration Access Point mode.
 */
void configModeCallback (WiFiManager *myWiFiManager) {
  clr();
  printStringWithShift("AP ", 60); // Show "AP" on the matrix to guide the user on-site
  refreshAll();
  Serial.println("Entered Wi-Fi Configuration AP Mode!");
}

void setup() 
{
  Serial.begin(115200);

  // Initialize MAX7219 communication and display parameters
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);  // Turn on display hardware
  sendCmdAll(CMD_INTENSITY, 3); // Set default display brightness level (0-15)

  // Banner intro sequence on boot
  printStringWithShift("Conference Timer     ", 60);
  clr();
  printStringWithShift("... ", 60);

  // Initialize WiFiManager captive portal routine
  WiFiManager wm;
  wm.setConfigPortalTimeout(180); // Auto-timeout after 3 minutes if no credentials are provided

  // Attach configuration fallback display notification callback
  wm.setAPCallback(configModeCallback);

  // Attempt automatic connection or spawn an access point named "Timer_Configuration"
  if (!wm.autoConnect("Timer_Configuration", "timer123")) {
    Serial.println("Configuration Timeout. Resetting system...");
    ESP.restart();
  }

  Serial.println("\nSuccessfully connected to Wi-Fi network!");
  Serial.print("Timer Device Local IP Address: ");
  Serial.println(WiFi.localIP());

  // Marquee scroll the generated IP address on the matrix display so users can find the panel URL
  String ipStr = WiFi.localIP().toString() + "     ";
  printStringWithShift(ipStr.c_str(), 60);

  // Start mDNS Responder allowing users to reach the configuration page via http://timer.local
  if (MDNS.begin("timer")) {
    Serial.println("mDNS responder active! Reach dashboard at: http://timer.local");
  } else {
    Serial.println("Error initializing mDNS responder!");
  }

  // Register HTTP routing handlers
  server.on("/", handleRoot);
  server.on("/command", handleCommand);
  server.begin();

  // Initialize WebSocket stream infrastructure
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  // Register services to mDNS directory lookup structure
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);

  Serial.println("Web servers and real-time streaming sockets established successfully!");

  lastActivityTime = millis(); // Benchmark initialization point for eco sleep tracking
}

void loop()
{
  server.handleClient(); // Process incoming HTTP client requests
  webSocket.loop();      // Maintain WebSocket connection heartbeats and frame parsing
  MDNS.update();         // Process background incoming mDNS discovery queries

  updateTime();          // Process precision time calculations and countdown changes

  // Control colon blink frequency (500 ms interval changes)
  if(millis() - dotTime > 500) {
    dotTime = millis();
    if (timerRunning && remainingSeconds >= 0) {
      dots = !dots;     // Flash the separator dots while the timer runs normally
    } else {
      dots = 1;        // Keep dots constantly lit if the timer is paused or negative overrun
    }
  }

  // Activity detection overrides: Keep display active if the countdown is currently running
  if (timerRunning) {
    lastActivityTime = millis();
    if (matrixOffTimeout) {
      matrixOffTimeout = false;
      sendCmdAll(CMD_SHUTDOWN, 1);
    }
  }

  // Energy Efficiency / Low-power mode check: Check for 5 minutes of continuous user inactivity (300,000 ms)
  if (!timerRunning && (millis() - lastActivityTime > 300000)) {
    if (!matrixOffTimeout) {
      matrixOffTimeout = true;
      clr();              // Clear display frame buffer
      refreshAll();
      sendCmdAll(CMD_SHUTDOWN, 0); // Put MAX7219 modules into low-power hardware shutdown mode
      Serial.println("Display switched off automatically due to system inactivity timeout.");
    }
  }

  // Render routine processing - skipped if the physical hardware is powered down
  if (!matrixOffTimeout) {
    
    // Time overrun alert mechanism: Flashes the whole screen if time runs into negative values
    if (timerRunning && remainingSeconds < 0) {
      if (millis() - flashTime > 500) {
        flashTime = millis();
        displayIsOn = !displayIsOn;
        sendCmdAll(CMD_SHUTDOWN, displayIsOn ? 1 : 0);
      }
    } else {
      // Re-engage standard lighting mode stability when values are safe
      if (!displayIsOn) {
        displayIsOn = true;
        sendCmdAll(CMD_SHUTDOWN, 1);
      }
    }

    // Dynamic dual-module hardware representation logic
    if (remainingSeconds >= 0) {
      int minutes = remainingSeconds / 60;
      int seconds = remainingSeconds % 60;
      
      // Adaptation layer for small 2-module screens (16x8 pixels fits only 2 numeric characters at a time):
      // - Displays only MM (minutes) with tracking blinking indicator when time > 1 minute
      // - Switches to direct SS (seconds) representation during the final crucial minute countdown
      if (minutes == 0) {
        showSimpleClock(seconds, false); // Show absolute raw seconds remaining without a tracking dot
      } else {
        showSimpleClock(minutes, true);  // Show tracking minutes remaining with active ticking dot
      }
    } else {
      // Negative overrun tracking cap: Caps graphic layout rendering to safe maximum negative value "-99"
      long totalSecondsMinus = abs(remainingSeconds);
      if (totalSecondsMinus > 99) totalSecondsMinus = 99; 
      showSimpleClock(totalSecondsMinus, false); 
    }
  }
}

/**
 * Evaluates execution elapsed intervals to precisely adjust timer metrics every 1000 milliseconds.
 */
void updateTime()
{
  unsigned long currentMillis = millis();
  if (timerRunning && currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;
    remainingSeconds--; // Decrement running countdown timeline
    sendTimeToWeb();    // Push fresh metrics update stream downstream immediately
  }
}

// =======================================================================
// LOW-LEVEL GRAPHICS ARCHITECTURE ENGINES (HARDWARE RENDERING LOGIC)
// =======================================================================

/**
 * Draws numbers and tracks control dot parameters inside the active buffer memory arrays.
 */
void showSimpleClock(int digit, bool dotpos) {
  dx = dy = 0; 
  clr(); 
  showDigit(digit / 10,  1, dig6x8); // Draw Tens digit starting at column index 1
  showDigit(digit % 10,  9, dig6x8); // Draw Units digit starting at column index 9
  
  if (dotpos == true) setCol(15, dots ? B10000000 : 0); // Render the single tracking bit dot state if required
  refreshAll(); // Commit the computed screen space buffer array into the MAX7219 cascading physical register path
}

/**
 * Decodes font mapping tables and translates structures into raw coordinate pixel bit arrays.
 */
void showDigit(char ch, int col, const uint8_t *data) {
  if (dy < -8 || dy > 8) return; 
  int len = pgm_read_byte(data); 
  int w = pgm_read_byte(data + 1 + ch * len); 
  col += dx;
  
  for (int i = 0; i < w; i++) {
    if (col + i >= 0 && col + i < 8 * NUM_MAX) { 
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i); 
      if (!dy) scr[col + i] = v; 
      else scr[col + i] |= dy > 0 ? v >> dy : v << -dy; 
    }
  }
}

/**
 * Sets raw register parameters on specific matrix vertical lines directly.
 */
void setCol(int col, byte v) { 
  if (dy < -8 || dy > 8) return; 
  col += dx; 
  if (col >= 0 && col < 8 * NUM_MAX) {
    if (!dy) scr[col] = v; 
    else scr[col] |= dy > 0 ? v >> dy : v << -dy; 
  }
}

/**
 * Populates font lookup indexing data into specific buffer segments.
 */
int showChar(char ch, const uint8_t *data) { 
  int len = pgm_read_byte(data); 
  int w = pgm_read_byte(data + 1 + ch * len); 
  for (int i = 0; i < w; i++) {
    scr[NUM_MAX * 8 + i] = pgm_read_byte(data + 1 + ch * len + 1 + i); 
  }
  scr[NUM_MAX * 8 + w] = 0; 
  return w; 
}

/**
 * Drives single-character marquee pixel scrolling shift execution routines.
 */
void printCharWithShift(unsigned char c, int shiftDelay) { 
  if (c < ' ' || c > '~'+25) return; 
  c -= 32; 
  int w = showChar(c, font); 
  for (int i = 0; i < w + 1; i++) { 
    delay(shiftDelay); 
    scrollLeft(); 
    refreshAll(); 
  } 
}

/**
 * Iterates through full textual char strings to compile dynamic scrolling display updates.
 */
void printStringWithShift(const char* s, int shiftDelay) { 
  while (*s) { 
    printCharWithShift(*s, shiftDelay); 
    s++; 
  } 
}
