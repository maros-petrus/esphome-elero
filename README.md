# Looking for a new maintainer

This project is looking for a new maintainer. After having invested way too much time in remote-controlling my blinds, I was finally able to run proper cables to all of them and to implement a centralised control system. The code for driving wired blinds, especially the RolTop-J series, is available [here](https://github.com/andyboeh/esphome-elero_wired).

# Elero Remote Control Component for ESPHome

This project is heavily based on the work of two other people:

  * All encryption/decryption structures copied from https://github.com/QuadCorei8085/elero_protocol (MIT)
  * All remote handling based on code from https://github.com/stanleypa/eleropy (GPLv3)

## Goal and Status

Ultimately, this component should allow you to control Elero blinds with the
bidirectional protocol directly from Home Assistant using an ESP32 with a CC1101
module attached. Apart from SPI (MISO, MOSI, SCK, CS) only GDO0 is required (in contrast to the other projects, GDO2 is not needed - it's not available on my module so I configured the CC1101 a bit differently).

The current code can transmit and simulate the TempoTel 2 that I have. Since some values are different to the two projects mentioned above, I'm not sure which/if this has an impact. For this reason, various aspects of the protocol can be configured on a per-cover basis (see the respective section).

Please be advised that this is very early development make, features might not work as intended!

If you like my work, consider sponsoring this project via [Github Sponsors](https://github.com/sponsors/andyboeh) or by acquiring one of my [Amazon Wishlist](https://www.amazon.de/hz/wishlist/ls/ROO2X0G63PCT?ref_=wl_share) items.

## Configuration

See the provided [example file](example.yaml) for the minimum configuration required to configure a blind. Some optional parameters can also be set that allow the tuning of your system:

```
external_components:
  - source: github://andyboeh/esphome-elero

spi:
  clk_pin: GPIO18
  mosi_pin: GPIO23
  miso_pin: GPIO19

elero:
  cs_pin: GPIO5
  gdo0_pin: GPIO26
  freq0: 0x7a
  freq1: 0x71
  freq2: 0x21

cover:
  - platform: elero
    blind_address: 0xa831e5
    channel: 4
    remote_address: 0xf0d008
    name: Schlafzimmer
    open_duration: 25s
    close_duration: 22s
    poll_interval: 5min
    supports_tilt: False
    payload_1: 0x00
    payload_2: 0x04
    pck_inf1: 0x6a
    pck_inf2: 0x00
    hop: 0x0a
    command_check: 0x00
    command_stop: 0x10
    command_up: 0x20
    command_down: 0x40
    command_tilt: 0x24
    command_check_len: 29
    command_control_len: 29
    control_payload_1: 0x00
    control_payload_2: 0x04
    control_pck_inf1: 0x6a
    control_pck_inf2: 0x00
    control_down_pck_inf2: 0x00
    control_hop: 0x0a
    control_backward_address: 0xa831e5
    control_forward_address: 0xf0d008
    control_short_dst: 0x04
```

### Section spi
  * `clk_pin`: The CLOCK pin for SPI communication
  * `mosi_pin`: The MOSI pin for SPI communication
  * `miso_pin`: The MISO pin for SPI communication

### Section elero
  * `cs_pin`: The CS pin for SPI communication
  * `gdo0_pin`: The GDO0 pin for SPI communication
  * `freq0`: Tune the frequency value set in the FREQ0 register if different from `0x7a` (Optional)
  * `freq1`: Tune the frequency value set in the FREQ1 register if different from `0x71` (Optional)
  * `freq2`: Tune the frequency value set in the FREQ2 register if different from `0x21` (Optional)

### Section cover
  * `blind_address`: The address of the blind you would like to control
  * `channel`: The channel of the blind you would like to control
  * `remote_address`: The address of the remote to simulate
  * `name`: The name of the cover
  * `open_duration`: For position control, stop the time it takes to open the cover (Optional)
  * `close_duration`: For position control, stop the time it takes to close the cover (Optional)
  * `poll_interval`: Configure the polling interval for status updates if different from `5min` (Optional)
  * `supports_tilt`: If the cover supports tilt, set to `True` (Optional)
  * `payload_1`: Configure the first payload byte if different from `0x00` (Optional)
  * `payload_2`: Configure the second payload byte if different from `0x04` (Optional)
  * `pck_inf1`: Configure the first packet info byte if different from `0x6a` (Optional)
  * `pck_inf2`: Configure the second packet info byte if different from `0x00` (Optional)
  * `hop`: Configure the Hop byte if different from `0x0a` (Optional)
  * `command_check`: Configure the command sent for getting the blind status if different from `0x00` (Optional)
  * `command_stop`: Configure the command sent for stopping the blind if different from `0x10` (Optional)
  * `command_up`: Configure the command sent for opening the blind if different from `0x20` (Optional)
  * `command_down`: Configure the command sent for closing the blind if different from `0x40` (Optional)
  * `command_tilt`: Configure the command sent for tilting the blind if different from `0x24` (Optional)
  * `command_check_len`: Configure the packet length used for status polling. Supported values are `29` and `27`, default is `29`. (Optional)
  * `command_control_len`: Configure the packet length used for UP, DOWN, STOP and TILT commands. Supported values are `29` and `27`, default is `29`. (Optional)
  * `control_payload_1` / `control_payload_2`: Override the first two payload bytes for control packets while leaving the check packet values untouched. (Optional)
  * `control_pck_inf1` / `control_pck_inf2`: Override the packet type bytes for control packets while leaving the check packet values untouched. (Optional)
  * `control_down_pck_inf2`: Override the second packet type byte only for the DOWN command. Useful for remotes where UP/STOP and DOWN use different `typ2` values. (Optional)
  * `control_hop`: Override the hop byte for control packets while leaving the check packet value untouched. (Optional)
  * `control_backward_address` / `control_forward_address`: Override the backward and forward addresses for control packets. (Optional)
  * `control_short_dst`: Override the one-byte destination field used by `len=27` control packets. (Optional)
  * `learn_remote_address`: Optional separate remote identity for the experimental pairing buttons. Set this to a new, unused 24-bit remote address when trying to teach ESPHome as an additional transmitter. (Optional)

## Experimental pairing buttons

The component includes an experimental button platform that can send the currently reverse-engineered learn-mode packets for the UniTec-868. This is intended for testing only.

```yaml
cover:
  - platform: elero
    id: rolladen_cover
    blind_address: 0x140f17
    channel: 33
    remote_address: 0x23ab01
    learn_remote_address: 0x23ab55
    name: Rolladen

button:
  - platform: elero
    name: "Rolladen Learn Start"
    cover_id: rolladen_cover
    learn_step: start
  - platform: elero
    name: "Rolladen Learn Up"
    cover_id: rolladen_cover
    learn_step: up
  - platform: elero
    name: "Rolladen Learn Down"
    cover_id: rolladen_cover
    learn_step: down
  - platform: elero
    name: "Rolladen Learn Finalize"
    cover_id: rolladen_cover
    learn_step: finalize
```

The intended flow is: power-cycle the receiver, trigger `Learn Start`, then use `Learn Up` and `Learn Down` at the same points where the physical remote would normally ask for confirmation. `Learn Finalize` is available as an extra step if the receiver expects a final marker packet.

## Getting the blind address and other values

You need to have an existing remote control configure and connected to to your blind. This component only supports faking an existing blind, it is not possible to learn it as a new remote. In order to accomplish this, start out with an empty configuration **and add a fake cover** (otherwise, you get a compile error). Then, enable logging and listen to your blind communication. Do the following:

  1. Select a blind. On a TempoTel 2, this triggers reading the blind state. You should see a message like the following in your log. If there are multiple lines, look for the line where `src`, `bwd` and `fwd` all have the same values.
  ```
  len=29, cnt=45, typ=0x6a, typ2=0x00, hop=0a, syst=01, chl=09, src=0x908bef, bwd=0x908bef, fwd=0x908bef, #dst=01, dst=e039c9, rssi=-84.0, lqi=47, crc= 1, payload=[0x00 0x04 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00]
  ```

  2. Look at the lines. You now know: 
    - `pck_inf1`=`0x6a`
    - `pck_inf2` = `0x00`
    - `hop` = `0x0a`
    - `channel` = `9`
    - `remote_address` = `0x908bef`
    - `blind_address` = `e039c9`
    - `payload_1` = `0x00`
    - `payload_2` = `0x04`
    - `command_check` = `0x00`
  3. Press the UP, DOWN and STOP buttons consecutively and check the log again. The very last byte of the payload doesn't matter, the fifth byte is the command to send. An exemplary UP command (`0x20`) looks like this:
  ```
    len=29, cnt=46, typ=0x6a, typ2=0x00, hop=0a, syst=01, chl=09, src=0x908bef, bwd=0x908bef, fwd=0x908bef, #dst=01, dst=e039c9, rssi=-84.0, lqi=47, crc= 1, payload=[0x00 0x04 0x00 0x00 0x20 0x00 0x00 0x00 0x00 0x40]
  ```
  Some remotes use `len=29` for both state polling and control commands. Others use `len=29` for state polling but `len=27` for UP, DOWN and STOP. In that case, set `command_check_len: 29` and `command_control_len: 27`. If the short control packet also uses different `typ`, `hop`, `payload_1`, `payload_2`, or addressing fields, use the `control_*` options to override just the control packet profile.

  4. Add all required information to the configuration file and check. Your blinds should start moving.

## Position Control

This implementation does not support intermediate positions. However, by estimating the time the cover is travelling, the cover can be stopped in any desired position. This feature is experimental and it might be off.

## Tilt Control

Any tilt value > 0 will send out the tilt command. At the moment, setting tilt to 0 will not send out any command as I do not have any blinds supporting tilt.

## Troubleshooting

  1. No log output when pressing buttons: Check that the wiring is correct. If that's fine, your frequency might have an offset. I had to set my module to the values `0xc0`, `0x71` and `0x21` whereas the default is `0x7a`, `0x71` and `0x21`.
  2. Blind control doesn't work: Carefully check all values and compare with the real remote. Apart from the `cnt` value, all values need to match!

## Tested configurations

This project was tested on two different configurations:

  1. D1 Mini ESP32 + 433MHz CC1101 module. Bad range and limited reception, but generally working.
  2. WT32-ETH01 + 868MHz CC1101 module from a canibalized RWE Smart Home central. Very good range and excellent reception.
