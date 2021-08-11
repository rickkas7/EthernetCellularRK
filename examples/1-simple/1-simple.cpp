#include "Particle.h"

#include "EthernetCellularRK.h"

// Optional logging
SerialLogHandler LogHandler(LOG_LEVEL_TRACE);

// SYSTEM_THREAD(ENABLED) and SYSTEM_MODE(SEMI_AUTOMATIC) are required
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);


void setup() {
    // Wait for a USB serial connection for up to 15 seconds
    // Only used during testing so all of the log messages can be seen
    waitFor(Serial.isConnected, 15000); delay(2000);

    // You must add this to your setup() function
    EthernetCellular::instance().setup();
}

void loop() {
    // You must add this to your loop() function
    EthernetCellular::instance().loop();
}

