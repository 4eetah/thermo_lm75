thermo-lm75
==========
Digital thermometer based on lm75ad chip with i2c interface  

Setup
=====
Make sure the i2c device is presented on the system  
If you run rpi-cm3 there are two i2c char devices /dev/i2c-0, /dev/i2c-1  
i2c-1 is preferable for ordinary stuff.  

In case i2c interface isn't enabled on you pi:  
echo -en "dtparam=i2c_arm=on\n" >> /boot/config.txt - enable i2c-1  
echo -en "dtparam=i2c_vc=on\n" >> /boot/config.txt - enable i2c-0   
reboot  

In case you don't have i2c_dev kernel module loaded init script will try to load it on start  

make && make install  
/etc/init.d/thermo_lm75.sh start  

The configuration file is at /etc/default/thermo_lm75.cfg  
To remove project from the system: make remove        
