ifconfig eth0 192.168.11.10 netmask 255.255.255.0
route add -net default gw 192.168.11.1 

cp www.tar.gz /home/httpd; 
gunzip /home/httpd/www.tar.gz;  
tar xf /home/httpd/www.tar -C /home/httpd;
rm /home/httpd/www.tar

