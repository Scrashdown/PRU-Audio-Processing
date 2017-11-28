# Script adapted from :
# http://www.dangtrinh.com/2015/05/sharing-internet-with-beaglebone-black.html

# Run this script on the BBB side to connect to the Host's internet

set -e

sudo ifconfig usb0 192.168.7.2
sudo route add default gw 192.168.7.1
sudo sh -c "echo \"nameserver 8.8.8.8\" >> /etc/resolv.conf"