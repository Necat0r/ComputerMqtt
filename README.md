# ComputerMqtt

## What it does
Controls power, monitor, and audio on a Windows PC via MQTT, and automatically publishes devices to Home Assistant.

## MQTT Topics

### Power
| Topic | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `computer/power` | status | `true` / `false` | Whether the computer is on |
| `computer/power/control` | control | `false` | Suspend the machine |

### Monitor
| Topic | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `computer/monitor` | status | `true` / `false` | Whether the monitor is on |
| `computer/monitor/control` | control | `true` / `false` | Turn the monitor on or off |

### Audio
| Topic | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `computer/audio/output` | status | `true` / `false` | Whether receiver (speakers) is the active output |
| `computer/audio/output/control` | control | `true` / `false` | Switch output device (`true` = receiver, `false` = headphones) |
| `computer/audio/volume` | status | `0`–`100` | Current volume level (%) |
| `computer/audio/volume/control` | control | `0`–`100` | Set volume level (%) |
| `computer/audio/mute` | status | `true` / `false` | Whether audio is muted |
| `computer/audio/mute/control` | control | `true` / `false` | Mute or unmute audio |

## Running

*Command line:*<br>
`computermqtt <host> <port>`

*With console:*<br>
`computermqtt --console <host> <port>`

### Autostart
It can also be installed to autostart with the system. A new `ComputerMqtt` registry key will be added at `Computer\HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run`

*Install:*<br>
`computermqtt --install <host> <port>`

*Uninstall:*<br>
`computermqtt --uninstall`
