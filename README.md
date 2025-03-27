
# Orange PI4 PWM FAN CONTROLLER












## Installation

Compile code:
```bash
gcc -o fancontrol fancontrol.c
```
Run code for testing:
```bash
  ./fancontrol &
```
To kill the process use
```bash
  kill "process ID"
```
Or just execute without *&* and exit with Ctrl+c
## Create daemon service:
sudo nano /etc/systemd/system/fancontrol.service
and add the following text




```bash
  [Unit]
Description=Fan Control Daemon
After=multi-user.target

[Service]
User=root
Group=root
ExecStart=/root/fancontrol/fancontrol
Nice=-1  # Use a moderate nice value (positive number) to reduce priority
LimitNOFILE=1048576
Restart=always
RestartSec=5s

[Install]
WantedBy=multi-user.target
```

 Reload systemd to recognize the new service
```bash
sudo systemctl daemon-reload
```
 Enable the service to start on boot
```bash
sudo systemctl enable fancontrol
```
 Start the service now
```bash
sudo systemctl start fancontrol
```
 Check the service status
```bash
sudo systemctl status fancontrol
```





## Configuration fansettings.config

Lower number == faster FAN RPM.
Modify fan curve to your liking.

```bash
pwm_period = 50000
temp_40 = 50000
temp_45 = 45000
temp_50 = 40000
temp_55 = 35000
temp_60 = 30000
temp_65 = 25000
temp_70 = 20000
temp_75 = 12000
temp_80 = 500
temp_85 = 1
temp_90 = 1
pwm_period = 50000
```

