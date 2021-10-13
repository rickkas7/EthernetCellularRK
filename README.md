# EthernetCellularRK

- Github: [https://github.com/rickkas7/EthernetCellularRK](https://github.com/rickkas7/EthernetCellularRK)
- License: MIT (can use in open-source or closed-source products including commercial products)

This class and library is used with Gen 3 cellular devices (Boron, B Series SoM) that also have Ethernet connectivity. It is used for the case that you want to default to Ethernet, but fall back to cellular if the device loses its Ethernet connection.

This requires additional hardware such as the Ethernet FeatherWing (Boron) or the B Series evaluation board or the equivalent WizNET W5500 circuitry on your own custom B Series Base Board.

Your data operations quota is the same for both cellular and Ethernet, but extra non-cloud data (TCP or UDP) are measured for cellular but not for Ethernet. Thus if you have a large amount of non-cloud data to external servers, Ethernet can reduce your costs.

This class is necessary because Device OS will default to Ethernet, however it will only fall back to cellular if Ethernet is unplugged (no link or no DHCP). The built-in behavior will not switch if the Ethernet LAN loses Internet connectivity and is only providing local LAN access. By adding this class, you can make the device fall back to cellular on losing Internet on the Ethernet LAN.

One caveat to this is that only one network interface can be active at a time. You should expect that it will take a minute or two to fall back to cellular and connect (depending on settings). The default is to check every 5 minutes if Ethernet has come back. The caveat to this is that cloud connectivity is lost during the retry attempts, so you don't want to make the period too short, but making it very long will cause you to stay on cellular longer than necessary.

In your main application file, include the library header:

```cpp
#include "EthernetCellularRK.h"
```

Optionally enable logging:

```cpp
SerialLogHandler LogHandler(LOG_LEVEL_TRACE);
```

You must use system thread enabled and SEMI_AUTOMATIC mode for this library to function properly!

```cpp
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);
```

This class is a singleton; you do not create one as a global, on the stack, or with new.

This class is a singleton; you do not create one as a global, on the stack, or with new.

From global application setup you must call: 
```cpp
EthernetCellular::instance().setup();
```

From global application loop you must call: 
```cpp
EthernetCellular::instance().loop();
```

If you want to override default settings, use methods like: 
```cpp
EthernetCellular::instance().withRetryEthernetPeriod(10min);
```

By default, the code replaces cyan blinking or breathing of the RGB status LED to yellow when on cellular backup. To not do this and keep the cyan color always, use: 
```cpp
EthernetCellular::instance().withCellularBackupColor(RGB_COLOR_CYAN);
```

## Members

The [full browsable API documentation, including internal methods and fields](https://rickkas7.github.io/EthernetCellularRK/) is available, however the important methods you are most likely to need are summarized below.

---

### void EthernetCellular::setup() 

Perform setup operations; call this from global application setup()

```
void setup()
```

You typically use EthernetCellular::instance().setup();

---

### void EthernetCellular::loop() 

Perform application loop operations; call this from global application loop()

```
void loop()
```

You typically use EthernetCellular::instance().loop();

---

### EthernetCellular & EthernetCellular::withEthernetKeepAlive(std::chrono::seconds value) 

Set an Ethernet keep-alive value (default: 25 seconds)

```
EthernetCellular & withEthernetKeepAlive(std::chrono::seconds value)
```

#### Parameters
* `value` The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.

When the network is Ethernet, a keep-alive is required to keep the UDP port forwarding active so the cloud can communicate with the device. This could be as short as 30 seconds or be minutes or even hours. Since the data in Ethernet is rarely metered, this defaults to 25 seconds but can be made longer on a site-specific basis.

If the limit is too long, at the end of the period the cloud will not longer be able to communicate with the device if the connection is otherwise idle.

If you are publishing, reading variables, or calling functions, this will reset the keep-alive timer so if you are frequently sending data they keep-alive may never be reached.

If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct like:

```cpp
int keepAliveSec = 300;

EthernetCellular::instance().withEthernetKeepAlive(std::chrono::seconds(keepAliveSec));
```

---

### int EthernetCellular::getEthernetKeepAlive() const 

Returns the Ethernet keep-alive value (in seconds)

```
int getEthernetKeepAlive() const
```

---

### EthernetCellular & EthernetCellular::withCellularKeepAlive(std::chrono::seconds value) 

Set an cellular keep-alive value (default: 23 minutes)

```
EthernetCellular & withCellularKeepAlive(std::chrono::seconds value)
```

#### Parameters
* `value` The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.

When the network is cellular, a keep-alive is required to keep the UDP port forwarding active so the cloud can communicate with the device. This rarely needs to be set when using the Particle SIM, but is almost certainly required for a 3rd-party SIM. With a 3rd-party SIM the value could be as low as 30 seconds, but could be a few minutes.

If the limit is too long, at the end of the period the cloud will not longer be able to communicate with the device if the connection is otherwise idle.

If you are publishing, reading variables, or calling functions, this will reset the keep-alive timer so if you are frequently sending data they keep-alive may never be reached.

If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct like:

```cpp
int keepAliveSec = 300;

EthernetCellular::instance().withEthernetKeepAlive(std::chrono::seconds(keepAliveSec));
```

---

### int EthernetCellular::getCellularKeepAlive() const 

Returns the cellular keep-alive value (in seconds)

```
int getCellularKeepAlive() const
```

---

### EthernetCellular & EthernetCellular::withRetryEthernetPeriod(std::chrono::milliseconds value) 

Set the period to try switching back to Ethernet from cellular. Default: 5 minutes.

```
EthernetCellular & withRetryEthernetPeriod(std::chrono::milliseconds value)
```

#### Parameters
* `value` The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.

Because the Ethernet and cellular networks cannot be on at the same time, after switching to cellular backup we need to periodically switch back to Ethernet to see if it's back up. This will interrupt network connectivity for a short period of time, so you don't want to make it too short. However, you also won't switch back until this period is reached, so you'll stay on backup longer than necessary if you set it too long. The default is 5 minutes.

If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct like:

```cpp
int retryEthernetSecs = 300;

EthernetCellular::instance().withRetryEthernetPeriod(std::chrono::milliseconds(retryEthernetSecs));
```

---

### int EthernetCellular::getRetryEthernetPeriod() const 

Returns the period to try switching back to Ethernet from cellular (in milliseconds)

```
int getRetryEthernetPeriod() const
```

---

### EthernetCellular & EthernetCellular::withCellularConnectTimeout(std::chrono::milliseconds value) 

Set the maximum time to connect to cellular (blinking green). Default: 5 minutes.

```
EthernetCellular & withCellularConnectTimeout(std::chrono::milliseconds value)
```

#### Parameters
* `value` The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.

The recommended value is from 5 to 10 minutes. Setting it to short values may prevent ever being able to connect to cellular.

If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct like:

```cpp
int timeoutMillisecs = 60000;

EthernetCellular::instance().withCellularConnectTimeout(std::chrono::milliseconds(timeoutMillisecs));
```

---

### int EthernetCellular::getCellularConnectTimeout() const 

Returns the maximum time to connect to cellular (blinking green) (in milliseconds)

```
int getCellularConnectTimeout() const
```

---

### EthernetCellular & EthernetCellular::withCellularCloudConnectTimeout(std::chrono::milliseconds value) 

Set the maximum time to connect to the cloud while connected to cellular (blinking cyan). Default: 2 minutes.

```
EthernetCellular & withCellularCloudConnectTimeout(std::chrono::milliseconds value)
```

#### Parameters
* `value` The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.

The default is 2 minutes. It's normally fast, but in areas with low signal strength it make take longer.

If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct like:

```cpp
int timeoutMillisecs = 60000;

EthernetCellular::instance().withCellularCloudConnectTimeout(std::chrono::milliseconds(timeoutMillisecs));
```

---

### int EthernetCellular::getCellularCloudConnectTimeout() const 

Returns the maximum time to connect to the cloud while connected to cellular (blinking cyan) (in milliseconds)

```
int getCellularCloudConnectTimeout() const
```

---

### EthernetCellular & EthernetCellular::withEthernetConnectTimeout(std::chrono::milliseconds value) 

Set the maximum time to connect to Ethernet (blinking green). Default: 30 seconds.

```
EthernetCellular & withEthernetConnectTimeout(std::chrono::milliseconds value)
```

#### Parameters
* `value` The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.

This should normally be really fast, but if for some reason your Ethernet network takes a long time to establish a connection you could make the timeout longer.

If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct like:

```cpp
int timeoutMillisecs = 60000;

EthernetCellular::instance().withEthernetConnectTimeout(std::chrono::milliseconds(timeoutMillisecs));
```

---

### int EthernetCellular::getEthernetConnectTimeout() const 

Returns the maximum time to connect to Ethernet (blinking green) (in milliseconds)

```
int getEthernetConnectTimeout() const
```

---

### EthernetCellular & EthernetCellular::withEthernetCloudConnectTimeout(std::chrono::milliseconds value) 

Set the maximum time to connect to the cloud while connected to Ethernet (blinking cyan). Default: 30 seconds.

```
EthernetCellular & withEthernetCloudConnectTimeout(std::chrono::milliseconds value)
```

#### Parameters
* `value` The value as a chrono-literal, such as 25s for 25 seconds or 5min for 5 minutes.

The default is 30 seconds. It should normally only take a few seconds.

If you want to pass in a regular integer (`int`) instead of a chrono literal, you can use a construct like:

```cpp
int timeoutMillisecs = 60000;

EthernetCellular::instance().withEthernetCloudConnectTimeout(std::chrono::milliseconds(timeoutMillisecs));
```

---

### int EthernetCellular::getEthernetCloudConnectTimeout() const 

Returns the maximum time to connect to the cloud while connected to Ethernet (blinking cyan) (in milliseconds)

```
int getEthernetCloudConnectTimeout() const
```

---

### EthernetCellular & EthernetCellular::withCellularBackupColor(uint32_t value) 

Sets the status LED color when using cellular backup, replacing blinking or breathing cyan (default: yellow)

```
EthernetCellular & withCellularBackupColor(uint32_t value)
```

#### Parameters
* `value` Color value, such as RGB_COLOR_CYAN, RGB_COLOR_YELLOW, or RGB_COLOR_ORANGE

When switching to cellular backup, the cloud connection color (typically blinking cyan, followed by breathing cyan) can be overridden. The default in this class is to use yellow (blinking yellow followed by breathing yellow) when on cellular backup so you can tell it's on backup, and that color is not currently used by Device OS.

If you don't want the status LED color to be overridden, make this call with no parameter or use `RGB_COLOR_CYAN` to use the Device OS default.

Note that this does not override the blinking green (connecting to network) which will be blinking green for both cellular and Ethernet, however it's normally not in this state for very long and there are only so many available colors.

---

### uint32_t EthernetCellular::getCellularBackupColor() const 

Returns the status LED color when using cellular backup.

```
uint32_t getCellularBackupColor() const
```

Value is an RGB color as a uint32_t of the form 0x00RRGGBB, matching the format of constants like RGB_COLOR_CYAN, RGB_COLOR_YELLOW, or RGB_COLOR_ORANGE.

---

### ActiveInterface EthernetCellular::getActiveInterface() const 

Returns the enumeration for the currently active interface (NONE, CELLULAR, or ETHERNET)

```
ActiveInterface getActiveInterface() const
```

---

### EthernetCellular & EthernetCellular::withInterfaceChangeCallback(std::function< void(ActiveInterface oldInterface, ActiveInterface newInterface)> callback) 

Sets a notification callback for when the active interface changes.

```
EthernetCellular & withInterfaceChangeCallback(std::function< void(ActiveInterface oldInterface, ActiveInterface newInterface)> callback)
```

You can only have one interface change callback.

---

## Version History

### 0.0.2 (2021-10-13)

- Added getActiveInterface() and withInterfaceChangeCallback()

### 0.0.1 (2021-08-11)

- Initial version
