# Setting up argon through BLE

When the argon is turned on for the first time, the machine will enter an indefinite waiting period until the configuration options have been set by the user through BLE. All the data will be saved, so this process only needs to completed at the first usage of the device unless the values need to be changed at some other time.

Download the `Bluefruit Connect` app and check the `Show unnamed devices` and `Must have UART Service` boxes. Hit connect on one of the devices, and the blue status light on the corresponding argon will turn on.

Once the BLE connection has been established, open the UART tab and hit the send key to send a blank message to the argon

The argon will give a prompt to enter the network SSID, send a blank message if the network credentials have already been saved on the machine. If an SSID was entered, it will also prompt for a password and will give the option to test the WiFi credentials

The argon will then give a prompt for the device DSID. To keep the current value, enter in a blank message

The next prompt will ask for the time between data collection in milliseconds. For example, a value of `1000` will make the argon collect accelerometer data every second. To keep the current value, enter in a blank message.

The final prompt will ask for the time between each connection to WiFi in seconds. Entering in a value of `60` will make the argon connect to WiFi and send the data every minute. To keep the current value, enter in an blank message.

Once all the data has been entered, the BLE connection will be disconnected and the argon will begin collecting data.