# RAK5205 Firmware with uBlox Airborne Mode Support

A fork of the [RAK5205 example code](https://github.com/RAKWireless/Products_practice_based_on_RUI_v2.0/tree/master/based%20on%20RAK811/RAK5205), using the RAK Wireless 'RUI' API and 'Online Compiler'. 

This fork configures the uBlox 7 GPS on the RAK5205 into Airborne 1G mode on GPS Init, so it should obtain GPS lock when at high altitudes.

Note that I do not recommend RAK Wireless products. With their recent firmware versions you have to use a horrible online compiler system, which makes development extremely painful. This repository only exists to configure what I did manage to get working.

**WARNING: This firmware has NOT been flight tested! Do not rely on it for tracking your flight (Not that you should be relying on anything LoRaWAN / TTN based for reliable tracking anyway...)**

### Contacts
* Mark Jessop - vk5qi at rfhead.net

(Please don't contact me about RAK Wireless Products, I won't be buying any more after my experiences with this horrible toolchain.)

## Compilation Notes
Since RAK Wireless has moved away from properly open-sourcing their code (WTF RAK Wireless, why did you do this?!!!!), you need to use their 'Online Compiler' system, which is available here: https://build.rakwireless.com/

* Visit https://build.rakwireless.com/  (for some reason it sometimes take ages for the login page to appear)
* Create an account, then log in.
* Zip up all the .c and .h files in this directory, and name the zip file something vaguely meaningful.
* On the online compiler page, select a board type of RAK811-H, and upload the zip file.
* Click 'Compile'.
* With any luck, compilation will succeed and a zip file containing the built binaries will be downloaded.

Yes, this build process sucks. RAK Wireless have really made a bad decision with this. I'm just thankful I got it to work *at all*. 

## Flashing Notes
* Use the flashing utility from here: https://docs.rakwireless.com/RUI/#download-firmware
* Flashing is *slow*.
* I found I had to re-plug the board for flashing to work reliably. Sometimes it will just crash out halfway through an upload. Try, try again.

## Configuration
From here it should be possible to configure the board to talk to The Things Network via AT commands as per the RAK5205 quick-start guide: https://docs.rakwireless.com/Product-Categories/WisTrio/RAK7205-5205/Quickstart/
