Relay data between a smart servo such as a Dynamixel and a websocket client.

You'll need an ESP32 and a circuit that corresponds to the half-duplex UART configuration
used by these servos.\
eg: https://emanual.robotis.com/docs/en/dxl/x/xl430-w250/#communication-circuit

Tested with an XL430-w250, using this fork of the DynamixelSDK:\
https://github.com/nsted/DynamixelSDK-websocket

Written Nov. 2024
by Nicholas Stedman, nick@devicist.com
