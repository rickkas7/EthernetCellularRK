#ifndef __ETHERNETCELLULARRK_H
#define __ETHERNETCELLULARRK_H

#include "Particle.h"

/**
 * This class and library is used with Gen 3 cellular devices (Boron, B Series SoM) that also have
 * Ethernet connectivity. It is used for the case that you want to default to Ethernet,
 * but fall back to cellular if the device loses its Ethernet connection.
 * 
 * This requires additional hardware such as the Ethernet FeatherWing (Boron) or the
 * B Series evaluation board or the equivalent WizNET W5500 circuitry on your own 
 * custom B Series Base Board.
 * 
 * Your data operations quota is the same for both cellular and Ethernet, but
 * extra non-cloud data (TCP or UDP) are measured for cellular but not for Ethernet.
 * Thus if you have a large amount of non-cloud data to external servers, Ethernet
 * can reduce your costs.
 * 
 * This class is necessary because Device OS will default to Ethernet, however it will
 * only fall back to cellular if Ethernet is unplugged (no link or no DHCP). The 
 * built-in behavior will not switch if the Ethernet LAN loses Internet connectivity and
 * is only providing local LAN access. By adding this class, you can make the device
 * fall back to cellular on losing Internet on the Ethernet LAN.
 * 
 * One caveat to this is that only one network interface can be active at a time. You should
 * expect that it will take a minute or two to fall back to cellular and connect (depending
 * on settings). The default is to check every 5 minutes if Ethernet has come back. The
 * caveat to this is that cloud connectivity is lost during the retry attempts, so you 
 * don't want to make the period too short, but making it very long will cause you to stay
 * on cellular longer than necessary.
 * 
 * In your main application file, include the library header:
 * 
 * ```
 * #include "EthernetCellularRK.h"
 * ```
 * 
 * Optionally enable logging:
 * 
 * ```
 * SerialLogHandler LogHandler(LOG_LEVEL_TRACE);
 * ```
 * 
 * You must use system thread enabled and SEMI_AUTOMATIC mode for this library to function properly!
 * 
 * ```
 * SYSTEM_THREAD(ENABLED);
 * SYSTEM_MODE(SEMI_AUTOMATIC);
 * ```
 * 
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * ```
 * EthernetCellular::instance().setup();
 * ```
 * 
 * From global application loop you must call:
 * ```
 * EthernetCellular::instance().loop();
 * ```
 * 
 * If you want to override default settings, use methods like:
 * ```
 * EthernetCellular::instance().withRetryEthernetPeriod(10min);
 * ```
 * 
 * By default, the code replaces cyan blinking or breathing of the RGB status LED to yellow 
 * when on cellular backup. To not do this and keep the cyan color always, use:
 * ```
 * EthernetCellular::instance().withCellularBackupColor(RGB_COLOR_CYAN);
 * ```
 * 
 */
class EthernetCellular {
public:
    enum class ActiveInterface : int {
        NONE = 0,
        ETHERNET,
        CELLULAR
    };

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use EthernetCellular::instance() to instantiate the singleton.
     */
    static EthernetCellular &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use EthernetCellular::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use EthernetCellular::instance().loop();
     */
    void loop();

    /**
     * @brief Set an Ethernet keep-alive value (default: 25 seconds)
     * 
     * @param value The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.
     * 
     * When the network is Ethernet, a keep-alive is required to keep the UDP port forwarding active so the
     * cloud can communicate with the device. This could be as short as 30 seconds or be minutes or even
     * hours. Since the data in Ethernet is rarely metered, this defaults to 25 seconds but can be made
     * longer on a site-specific basis.
     * 
     * If the limit is too long, at the end of the period the cloud will not longer be able to communicate
     * with the device if the connection is otherwise idle.
     * 
     * If you are publishing, reading variables, or calling functions, this will reset the keep-alive timer
     * so if you are frequently sending data they keep-alive may never be reached.
     * 
     * If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct
     * like:
     * 
     * ```
     * int keepAliveSec = 300;
     * 
     * EthernetCellular::instance().withEthernetKeepAlive(std::chrono::seconds(keepAliveSec));
     * ```
     */
    EthernetCellular &withEthernetKeepAlive(std::chrono::seconds value) { ethernetKeepAlive = value; return *this; };

    /**
     * @brief Returns the Ethernet keep-alive value (in seconds)
     */
    int getEthernetKeepAlive() const { return (int)ethernetKeepAlive.count(); };

    /**
     * @brief Set an cellular keep-alive value (default: 23 minutes)
     * 
     * @param value The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.
     * 
     * When the network is cellular, a keep-alive is required to keep the UDP port forwarding active so the
     * cloud can communicate with the device. This rarely needs to be set when using the Particle SIM, 
     * but is almost certainly required for a 3rd-party SIM. With a 3rd-party SIM the value could be as low 
     * as 30 seconds, but could be a few minutes.
     * 
     * If the limit is too long, at the end of the period the cloud will not longer be able to communicate
     * with the device if the connection is otherwise idle.
     * 
     * If you are publishing, reading variables, or calling functions, this will reset the keep-alive timer
     * so if you are frequently sending data they keep-alive may never be reached.
     * 
     * If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct
     * like:
     * 
     * ```
     * int keepAliveSec = 300;
     * 
     * EthernetCellular::instance().withEthernetKeepAlive(std::chrono::seconds(keepAliveSec));
     * ```
     */
    EthernetCellular &withCellularKeepAlive(std::chrono::seconds value) { cellularKeepAlive = value; return *this; };

    /**
     * @brief Returns the cellular keep-alive value (in seconds)
     */
    int getCellularKeepAlive() const { return (int)cellularKeepAlive.count(); };

    /**
     * @brief Set the period to try switching back to Ethernet from cellular. Default: 5 minutes.
     * 
     * @param value The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.
     * 
     * Because the Ethernet and cellular networks cannot be on at the same time, after switching to cellular
     * backup we need to periodically switch back to Ethernet to see if it's back up. This will interrupt
     * network connectivity for a short period of time, so you don't want to make it too short. However,
     * you also won't switch back until this period is reached, so you'll stay on backup longer than 
     * necessary if you set it too long. The default is 5 minutes.
     * 
     * If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct
     * like:
     * 
     * ```
     * int retryEthernetSecs = 300;
     * 
     * EthernetCellular::instance().withRetryEthernetPeriod(std::chrono::milliseconds(retryEthernetSecs));
     * ```
     */ 
    EthernetCellular &withRetryEthernetPeriod(std::chrono::milliseconds value) { retryEthernetPeriod = value; return *this; };

    /**
     * @brief Returns the  period to try switching back to Ethernet from cellular (in milliseconds)
     */
    int getRetryEthernetPeriod() const { return (int)retryEthernetPeriod.count(); };

    /**
     * @brief Set the maximum time to connect to cellular (blinking green). Default: 5 minutes.
     * 
     * @param value The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.
     * 
     * The recommended value is from 5 to 10 minutes. Setting it to short values may prevent ever being able to connect
     * to cellular.
     * 
     * If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct
     * like:
     * 
     * ```
     * int timeoutMillisecs = 60000;
     * 
     * EthernetCellular::instance().withCellularConnectTimeout(std::chrono::milliseconds(timeoutMillisecs));
     * ```
     */ 
    EthernetCellular &withCellularConnectTimeout(std::chrono::milliseconds value) { cellularConnectTimeout = value; return *this; };

    /**
     * @brief Returns the maximum time to connect to cellular (blinking green) (in milliseconds)
     */
    int getCellularConnectTimeout() const { return (int)cellularConnectTimeout.count(); };

    /**
     * @brief Set the maximum time to connect to the cloud while connected to cellular (blinking cyan). Default: 2 minutes.
     * 
     * @param value The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.
     * 
     * The default is 2 minutes. It's normally fast, but in areas with low signal strength it make take longer.
     * 
     * If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct
     * like:
     * 
     * ```
     * int timeoutMillisecs = 60000;
     * 
     * EthernetCellular::instance().withCellularCloudConnectTimeout(std::chrono::milliseconds(timeoutMillisecs));
     * ```
     */ 
    EthernetCellular &withCellularCloudConnectTimeout(std::chrono::milliseconds value) { cellularCloudConnectTimeout = value; return *this; };

    /**
     * @brief Returns the maximum time to connect to the cloud while connected to cellular (blinking cyan) (in milliseconds)
     */
    int getCellularCloudConnectTimeout() const { return (int)cellularCloudConnectTimeout.count(); };

    /**
     * @brief Set the maximum time to connect to Ethernet (blinking green). Default: 30 seconds.
     * 
     * @param value The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.
     * 
     * This should normally be really fast, but if for some reason your Ethernet network takes a long
     * time to establish a connection you could make the timeout longer.
     * 
     * If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct
     * like:
     * 
     * ```
     * int timeoutMillisecs = 60000;
     * 
     * EthernetCellular::instance().withEthernetConnectTimeout(std::chrono::milliseconds(timeoutMillisecs));
     * ```
     */ 
    EthernetCellular &withEthernetConnectTimeout(std::chrono::milliseconds value) { ethernetConnectTimeout = value; return *this; };

    /**
     * @brief Returns the maximum time to connect to Ethernet (blinking green) (in milliseconds)
     */
    int getEthernetConnectTimeout() const { return (int)ethernetConnectTimeout.count(); };

    /**
     * @brief Set the maximum time to connect to the cloud while connected to Ethernet (blinking cyan). Default: 30 seconds.
     * 
     * @param value The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.
     * 
     * The default is 30 seconds. It should normally only take a few seconds.
     * 
     * If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct
     * like:
     * 
     * ```
     * int timeoutMillisecs = 60000;
     * 
     * EthernetCellular::instance().withEthernetCloudConnectTimeout(std::chrono::milliseconds(timeoutMillisecs));
     * ```
     */ 
    EthernetCellular &withEthernetCloudConnectTimeout(std::chrono::milliseconds value) { ethernetCloudConnectTimeout = value; return *this; };

    /**
     * @brief Returns the maximum time to connect to the cloud while connected to Ethernet (blinking cyan) (in milliseconds)
     */
    int getEthernetCloudConnectTimeout() const { return (int)ethernetCloudConnectTimeout.count(); };

    /**
     * @brief Sets the status LED color when using cellular backup, replacing blinking or breathing cyan (default: yellow)
     * 
     * @param value Color value, such as RGB_COLOR_CYAN, RGB_COLOR_YELLOW, or RGB_COLOR_ORANGE
     * 
     * When switching to cellular backup, the cloud connection color (typically blinking cyan, followed
     * by breathing cyan) can be overridden. The default in this class is to use yellow (blinking yellow
     * followed by breathing yellow) when on cellular backup so you can tell it's on backup, and that 
     * color is not currently used by Device OS.
     * 
     * If you don't want the status LED color to be overridden, make this call with no parameter or
     * use `RGB_COLOR_CYAN` to use the Device OS default.
     * 
     * Note that this does not override the blinking green (connecting to network) which will be blinking green
     * for both cellular and Ethernet, however it's normally not in this state for very long and there are
     * only so many available colors.
     */
    EthernetCellular &withCellularBackupColor(uint32_t value = RGB_COLOR_CYAN) { cellularBackupColor = value; return *this; };

    /**
     * @brief Returns the status LED color when using cellular backup
     * 
     * Value is an RGB color as a uint32_t of the form 0x00RRGGBB, matching the format of constants like
     * RGB_COLOR_CYAN, RGB_COLOR_YELLOW, or RGB_COLOR_ORANGE.
     */
    uint32_t getCellularBackupColor() const { return cellularBackupColor; };

    /**
     * @brief Returns the enumeration for the currently active interface (NONE, CELLULAR, or ETHERNET)
     */
    ActiveInterface getActiveInterface() const { return activeInterface; };

    /**
     * @brief Sets a notification callback for when the active interface changes
     * 
     * You can only have one interface change callback.
     */
    EthernetCellular &withInterfaceChangeCallback(std::function<void(ActiveInterface oldInterface, ActiveInterface newInterface)> callback) {
        interfaceChangeCallback = callback;
        return *this;
    }

protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use EthernetCellular::instance() to instantiate the singleton.
     */
    EthernetCellular();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~EthernetCellular();

    /**
     * This class is a singleton and cannot be copied
     */
    EthernetCellular(const EthernetCellular&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    EthernetCellular& operator=(const EthernetCellular&) = delete;

    /**
     * @brief Sets the active interface and calls the notification handler if necessary
     * 
     * Internal use only
     */
    void setActiveInterface(ActiveInterface newActiveInterface);

    /**
     * @brief Starting state at boot
     * 
     * Next state:
     * - stateTryEthernet if Ethernet is present
     * - stateTryCellular if no Ethernet is present
     */
    void stateStart();

    /**
     * @brief Starting point for switching to Ethernet
     * 
     * - Disconnects from Cellular
     * - Connects to Ethernet
     * - Sets the cloud connection RGB status LED theme to default (cyan)
     * 
     * Next State:
     * - stateWaitEthernetReady (always)
     */
    void stateTryEthernet();

    /**
     * @brief Waits until Ethernet is ready or times out
     * 
     * If ready:
     * - Does a Particle.connect to reconnect to the Particle cloud
     * 
     * Next State:
     * - stateWaitEthernetCloud if ready
     * - stateTryCellular if ethernetConnectTimeout is reached without ready
     */
    void stateWaitEthernetReady();

    /**
     * @brief Waits until cloud connected while on Ethernet
     * 
     * If connected;
     * - Sets the keep-alive to ethernetKeepAlive
     * 
     * Next State:
     * - stateEthernetCloudConnected if connected to the Particle cloud
     * - stateTryCellular if timeout of ethernetCloudConnectTimeout is reached
     * 
     * This is separate from connecting to Ethernet, because it's possible for 
     * Ethernet to be ready if the local LAN is up, but not be able to reach 
     * the Particle cloud if the Internet connection to the LAN is down. 
     */
    void stateWaitEthernetCloud();

    /**
     * @brief State for when cloud connected
     * 
     * Next State;
     * - stateWaitEthernetCloud if cloud connection is lost
     */
    void stateEthernetCloudConnected();


    /**
     * @brief Starting point for switching to cellular
     * 
     * - Disconnects from Ethernet
     * - Connects to cellular
     * - Sets the cloud connection RGB status LED theme
     * 
     * Next State:
     * - stateWaitCellularReady (always)
     */
    void stateTryCellular();


    /**
     * @brief Waits until cellular is ready or times out
     * 
     * If ready:
     * - Does a Particle.connect to reconnect to the Particle cloud
     * 
     * Next State:
     * - stateWaitCellularCloud if ready
     * - stateTryEthernet if cellularConnectTimeout is reached without ready and ethernetPresent is true
     * 
     * If there is no Ethernet controller, this state will be remained in forever until
     * connected to cellular.
     */
    void stateWaitCellularReady();

    /**
     * @brief Waits until cloud connected while on cellular
     * 
     * If connected;
     * - Sets the keep-alive to cellularKeepAlive
     * 
     * Next State:
     * - stateCellularCloudConnected if connected to the Particle cloud
     * - stateTryEthernet if cellularConnectTimeout is reached without ready and ethernetPresent is true
     */
    void stateWaitCellularCloud();

    /**
     * @brief State for when cloud is connected via cellular
     * 
     * Next State;
     * - stateWaitCellularCloud if cloud connection is lost
     * - stateCellularWaitDisconnectedThenTryEthernet if retryEthernetPeriod is reached and ethernetPresent is true
     */
    void stateCellularCloudConnected();

    /**
     * @brief Waits for Particle.connected to be false then tries Ethernet again
     * 
     * Next state: 
     * - stateTryEthernet after Particle.connected() is false
     */
    void stateCellularWaitDisconnectedThenTryEthernet();

    /**
     * @brief State handler method
     * 
     * This is one of the protected methods like stateStart.
     */
    std::function<void(EthernetCellular &)> stateHandler = &EthernetCellular::stateStart;

    /**
     * @brief Set during stateStart if Ethernet hardware is detected
     */
    bool ethernetPresent = false;

    /**
     * @brief Used for determining how long to wait in a state
     * 
     * All calculations using stateTime are safe for when millis rolls over to 0 every 49 days.
     */
    unsigned long stateTime = 0;

    /**
     * @brief Ethernet keep-alive value (default: 25 seconds)
     * 
     * When the network is Ethernet, a keep-alive is required to keep the UDP port forwarding active so the
     * cloud can communicate with the device. This could be as short as 30 seconds or be minutes or even
     * hours. Since the data in Ethernet is rarely metered, this defaults to 25 seconds but can be made
     * longer on a site-specific basis.
     * 
     * If the limit is too long, at the end of the period the cloud will not longer be able to communicate
     * with the device if the connection is otherwise idle.
     * 
     * If you are publishing, reading variables, or calling functions, this will reset the keep-alive timer
     * so if you are frequently sending data they keep-alive may never be reached.
     */    
    std::chrono::seconds ethernetKeepAlive = 25s;

    /**
     * @brief Cellular keep-alive value (default: 23 minutes)
     * 
     * When the network is cellular, a keep-alive is required to keep the UDP port forwarding active so the
     * cloud can communicate with the device. This rarely needs to be set when using the Particle SIM, 
     * but is almost certainly required for a 3rd-party SIM. With a 3rd-party SIM the value could be as low 
     * as 30 seconds, but could be a few minutes.
     * 
     * If the limit is too long, at the end of the period the cloud will not longer be able to communicate
     * with the device if the connection is otherwise idle.
     * 
     * If you are publishing, reading variables, or calling functions, this will reset the keep-alive timer
     * so if you are frequently sending data they keep-alive may never be reached.
     */
    std::chrono::seconds cellularKeepAlive = 23min;

    /**
     * Period to try switching back to Ethernet from cellular. Default: 5 minutes.
     *      * 
     * Because the Ethernet and cellular networks cannot be on at the same time, after switching to cellular
     * backup we need to periodically switch back to Ethernet to see if it's back up. This will interrupt
     * network connectivity for a short period of time, so you don't want to make it too short. However,
     * you also won't switch back until this period is reached, so you'll stay on backup longer than 
     * necessary if you set it too long. The default is 5 minutes.
     */ 
    std::chrono::milliseconds retryEthernetPeriod = 5min;

    /**
     * @brief The maximum time to connect to cellular (blinking green). Default: 5 minutes.
     * 
     * 
     * The recommended value is from 5 to 10 minutes. Setting it to short values may prevent ever being able to connect
     * to cellular.
     */ 
    std::chrono::milliseconds cellularConnectTimeout = 5min;

    /**
     * @brief The maximum time to connect to the cloud while connected to cellular (blinking cyan). Default: 2 minutes.
     *      * 
     * The default is 2 minutes. It's normally fast, but in areas with low signal strength it make take longer.
     */     
    std::chrono::milliseconds cellularCloudConnectTimeout = 2min;

    /**
     * @brief The maximum time to connect to Ethernet (blinking green). Default: 30 seconds.
     * 
     * This should normally be really fast, but if for some reason your Ethernet network takes a long
     * time to establish a connection you could make the timeout longer.
     */ 
    std::chrono::milliseconds ethernetConnectTimeout = 30s;

    /**
     * @brief The maximum time to connect to the cloud while connected to Ethernet (blinking cyan). Default: 30 seconds.
     *  
     * The default is 30 seconds. It should normally only take a few seconds.
     */     
    std::chrono::milliseconds ethernetCloudConnectTimeout = 30s;

    /**
     * @brief The status LED color when using cellular backup, replacing blinking or breathing cyan (default: yellow)
     * 
     * When switching to cellular backup, the cloud connection color (typically blinking cyan, followed
     * by breathing cyan) can be overridden. The default in this class is to use yellow (blinking yellow
     * followed by breathing yellow) when on cellular backup so you can tell it's on backup, and that 
     * color is not currently used by Device OS.
     * 
     * If you don't want the status LED color to be overridden, make this call with no parameter or
     * use `RGB_COLOR_CYAN` to use the Device OS default.
     * 
     * Note that this does not override the blinking green (connecting to network) which will be blinking green
     * for both cellular and Ethernet, however it's normally not in this state for very long and there are
     * only so many available colors.
     */    
    uint32_t cellularBackupColor = RGB_COLOR_YELLOW;

    /**
     * @brief The currently active interface
     */
    ActiveInterface activeInterface = ActiveInterface::NONE;

    /**
     * @brief Optional callback function to call when the active interface changes
     */
    std::function<void(ActiveInterface oldInterface, ActiveInterface newInterface)> interfaceChangeCallback = NULL;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static EthernetCellular *_instance;
};

#endif /* __ETHERNETCELLULARRK_H */