# Cloudflare-Application-Systems
Small Ping CLI Application

The program takes in 1 argument (hostname or IP address). It will incur an infinite loop of echo requests. In each cycle, the latency and packet loss is reported.

How to Run:

g++ ping.cpp -o ping

sudo ./ping [arg1]
