#include "EthernetCellularRK.h"

EthernetCellular *EthernetCellular::_instance;

static Logger _log("app.ethcell");

#if Wiring_WiFi
#define CellularOrWiFi WiFi
#endif
#if Wiring_Cellular
#define CellularOrWiFi Cellular
#endif


// [static]
EthernetCellular &EthernetCellular::instance() {
    if (!_instance) {
        _instance = new EthernetCellular();
    }
    return *_instance;
}

EthernetCellular::EthernetCellular() {
}

EthernetCellular::~EthernetCellular() {
}

void EthernetCellular::setup() {
}

void EthernetCellular::loop() {
    stateHandler(*this);
}


void EthernetCellular::setActiveInterface(ActiveInterface newActiveInterface) {
    ActiveInterface oldActiveInterface = activeInterface;
    activeInterface = newActiveInterface;

    if (oldActiveInterface != newActiveInterface) {
        if (interfaceChangeCallback) {
            interfaceChangeCallback(oldActiveInterface, newActiveInterface);
        }
    }
}


void EthernetCellular::stateStart() {
    if (!System.featureEnabled(FEATURE_ETHERNET_DETECTION)) {
        _log.info("FEATURE_ETHERNET_DETECTION enabled (was disabled before)");
        System.enableFeature(FEATURE_ETHERNET_DETECTION);
    }

    uint8_t mac[6];
    if (Ethernet.macAddress(mac) != 0) {
        _log.info("Ethernet adapter present");

        ethernetPresent = true;
    }
    else {
        // the macAddress function returns 0 if there is no Ethernet adapter present
        _log.info("No Ethernet adapter");
    }

    if (ethernetPresent) {
        stateHandler = &EthernetCellular::stateTryEthernet;
    }
    else {        
        stateHandler = &EthernetCellular::stateTryCellular;
    }
}
void EthernetCellular::stateTryEthernet() {
    _log.info("Trying to connect by Ethernet");
    LEDSystemTheme::restoreDefault();
    setActiveInterface(ActiveInterface::NONE);

    stateTime = millis();
    CellularOrWiFi.disconnect();
    Ethernet.connect();
    stateHandler = &EthernetCellular::stateWaitEthernetReady;
}

void EthernetCellular::stateWaitEthernetReady() {
    if (Ethernet.ready()) {
        // Have Ethernet link, try connecting to the Particle cloud
        Particle.connect();
        stateHandler = &EthernetCellular::stateWaitEthernetCloud;
        return;
    }
    if (millis() - stateTime >= ethernetConnectTimeout.count()) {
        // Timed out connecting to Ethernet (no DHCP, for example)
        _log.info("Timed out connecting to Ethernet, reverting to Cellular");
        stateHandler = &EthernetCellular::stateTryCellular;
        return;
    }
    // Wait some more
}

void EthernetCellular::stateWaitEthernetCloud() {
    if (Particle.connected()) {
        _log.info("Cloud connected over Ethernet keepAlive=%d", (int) ethernetKeepAlive.count());
        setActiveInterface(ActiveInterface::ETHERNET);
        Particle.keepAlive(ethernetKeepAlive.count());
        stateTime = millis();
        stateHandler = &EthernetCellular::stateEthernetCloudConnected;
        return;  
    }

    if (millis() - stateTime >= ethernetCloudConnectTimeout.count()) {
        _log.info("Took too long to connect to the cloud by Ethernet, switching to cellular");
        Particle.disconnect();
        Ethernet.disconnect();
        stateHandler = &EthernetCellular::stateTryCellular;
        return;
    }

    // Wait some more
}

void EthernetCellular::stateEthernetCloudConnected() {
    if (!Particle.connected()) {
        _log.info("Disconnected from the cloud while on Ethernet, waiting for reconnect");
        stateHandler = &EthernetCellular::stateWaitEthernetCloud;
        stateTime = millis();
        return;
    }       

}


void EthernetCellular::stateTryCellular() {
    _log.info("Trying to connect by cellular");
    setActiveInterface(ActiveInterface::NONE);

    // When in cellular backup mode, show breathing yellow instead of breathing cyan when cloud connected
    if (cellularBackupColor != RGB_COLOR_CYAN) {
        LEDSystemTheme theme;
        theme.setColor(LED_SIGNAL_CLOUD_CONNECTING, cellularBackupColor);
        theme.setColor(LED_SIGNAL_CLOUD_HANDSHAKE, cellularBackupColor);
        theme.setColor(LED_SIGNAL_CLOUD_CONNECTED, cellularBackupColor);
        theme.apply(); 
    }
    else {
        LEDSystemTheme::restoreDefault();
    }

    stateTime = millis();
    Ethernet.disconnect();
    CellularOrWiFi.connect();
    stateHandler = &EthernetCellular::stateWaitCellularReady;
}

void EthernetCellular::stateWaitCellularReady() {
    if (CellularOrWiFi.ready()) {
        // Have Ethernet link, try connecting to the Particle cloud
        Particle.connect();
        stateHandler = &EthernetCellular::stateWaitCellularCloud;
        return;
    }
    if (millis() - stateTime >= cellularConnectTimeout.count()) {
        // Timed out connecting to Cellular (no tower, for example)
        if (ethernetPresent) {
            // Try going back to Ethernet
            stateHandler = &EthernetCellular::stateTryEthernet;
            return;
        }
        // No Ethernet to go back to, just keep waiting
    }

    // Wait some more    
}

void EthernetCellular::stateWaitCellularCloud() {
    if (Particle.connected()) {
        _log.info("Cloud connected over cellular keepAlive=%d", (int) cellularKeepAlive.count());
        Particle.keepAlive(cellularKeepAlive.count());
        setActiveInterface(ActiveInterface::CELLULAR);

        stateTime = millis();
        stateHandler = &EthernetCellular::stateCellularCloudConnected;
        return;  
    }

    if (millis() - stateTime >= cellularCloudConnectTimeout.count()) {
        if (ethernetPresent) {
            _log.info("Took too long to connect to the cloud by Cellular, trying Ethernet again");
            Particle.disconnect();

            stateHandler = &EthernetCellular::stateTryEthernet;
            return;
        }
    }

    // Wait some more
}

void EthernetCellular::stateCellularCloudConnected() {
    if (!Particle.connected()) {
        _log.info("Disconnected from the cloud while on Cellular");
        stateHandler = &EthernetCellular::stateWaitCellularCloud;
        stateTime = millis();
        return;
    }

    if (millis() - stateTime >= retryEthernetPeriod.count()) {
        if (ethernetPresent) {
            _log.info("Trying Ethernet again");
            Particle.disconnect();
            setActiveInterface(ActiveInterface::NONE);

            // We were really cloud connected before, so disconnecting will take
            // a non-zero amount of time. This does not happen when going
            // from Ethernet to cellular for a failed connection, as the
            // connection hasn't been made yet so there's nothing to tear down.
            stateHandler = &EthernetCellular::stateCellularWaitDisconnectedThenTryEthernet;
            return;
        }
    }

}

void EthernetCellular::stateCellularWaitDisconnectedThenTryEthernet() {
    if (Particle.disconnected()) {
        stateHandler = &EthernetCellular::stateTryEthernet;
        return;
    }
}



