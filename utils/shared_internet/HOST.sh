# Script adapted from :
# http://www.dangtrinh.com/2015/05/sharing-internet-with-beaglebone-black.html

# Run this script on the host side to share internet with the BBB
# /!\ You will probably need to change the interfaces names first

INTERNET_INTERFACE="wlp2s0"
BBB_INTERFACE="enp62s0u1"

sudo ifconfig $BBB_INTERFACE 192.168.7.1
sudo iptables --table nat --append POSTROUTING --out-interface $INTERNET_INTERFACE -j MASQUERADE
sudo iptables --append FORWARD --in-interface $BBB_INTERFACE -j ACCEPT
sudo sh -c "echo 1 > /proc/sys/net/ipv4/ip_forward"