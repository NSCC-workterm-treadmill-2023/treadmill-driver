# Touchscreen setup

1. Configure the ethernet port to have static IP 192.168.5.1
1. Install [Mosquitto](https://mosquitto.org/) MQTT broker to `C:\Program Files\mosquitto` (the default location)
1. Give it the following configuration in mosquitto.conf:

```
listener 1883 0.0.0.0
allow_anonymous true
```

The Nucleo uses 192.168.5.2.

## Security concerns

* Mosquitto is currently wide open to the world. It should be locked down to localhost (for the web backend) and 192.168.5.1 (for the Nucleo)
* Authentication and TLS should be configured for MQTT, if the Arduino lib supports that.
* The network is currently 192.168.5.0/24. If we made it /30, then an attacker couldn't get an IP, because the only available ones would be in use.
