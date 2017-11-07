# Create symlinks to binaries for CLPRU and LNKPRU
PRU_CGT=/usr/share/ti/cgt-pru
sudo mkdir $PRU_CGT/bin
sudo ln -s /usr/bin/clpru $PRU_CGT/bin/clpru
sudo ln -s /usr/bin/lnkpru $PRU_CGT/bin/lnkpru

# Create env. variable for CLPRU
NEWLINE="export PRU_CGT=$PRU_CGT"
echo "NEWLINE=$NEWLINE"
echo $NEWLINE >> /home/debian/.bashrc
sudo sh -c "echo $NEWLINE >> /root/.bashrc"
