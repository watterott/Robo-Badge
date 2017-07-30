# Robo-Badge
The examples can be compiled and uploaded using the Arduino IDE.

## Files
* [Arduino IDE](http://arduino.cc/en/Main/Software)
* [Arduino Boards Package (ATtiny841)](https://github.com/watterott/Arduino-Boards#watterott-boards-package)
* [Examples](https://github.com/watterott/Robo-Badge/archive/master.zip)


## Installation
* Download and install the [Arduino IDE](http://arduino.cc/en/Main/Software).
* Add the following URLs to the Arduino Boards Manager (*File->Preferences*):
```
https://github.com/watterott/Arduino-Boards/raw/master/package_watterott_index.json
```
* Install the **Watterott AVR Boards** via the Boards Manager (*Tools->Boards->Boards Manager*). Further infos [here](https://github.com/watterott/Arduino-Boards#watterott-boards-package).
* Connect the Robo-Badge via USB to the computer and then press the reset button.
* Now a LED on the Robo-Badge should start to blink.
* On a Windows operating system a driver installation is needed. The drivers are included with the BSP or you can find them [here](https://github.com/watterott/Arduino-Boards/raw/master/files/micronucleus_driver.zip).


## Build and Upload
* Start the Arduino IDE.
* Open the respective Arduino Sketch ```.ino```.
* Select **ATtiny841 (8MHz)** under **Tools->Board**.
* Start build and upload: **File->Upload**.
* When the compiling has finished and *Uploading...* is shown, press the **reset switch** to start the bootloader.
* Wait till the upload has finished and *Done uploading* is shown.
