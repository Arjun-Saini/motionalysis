# Setting up argon through BLE

Once the BLE connection has been established, open the UART tab and hit the send key to send a blank value to the argon

The argon will give a prompt to enter the network SSID, enter in ```0``` if the network credentials have already been saved on the machine. If an SSID was entered, it will also prompt for a password

The argon will then give a prompt for the device DSID, enter in ```0``` if it has already been saved

The next prompt will ask for the time between data collection in milliseconds, enter in ```0``` to use the default value of ```1000```; a value of ```1000``` will make the argon collect accelerometer data every second

The final prompt will ask for the time between each connection to WiFi in milliseconds, enter in ```0``` to use the default value of ```60000```; a value of ```60000``` will make the argon connect to WiFi every minute