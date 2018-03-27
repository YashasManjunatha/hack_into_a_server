To hack the server, we constructed a stack-smash attack using a modified version of the shellcode linked to in the project document. First, we constructed a useful shellcode to hack a local copy of the webserver, using gdb to modify the values of the return address we wished to overwrite. After performing a successful hack on the local server and acknowledging that this address would be slightly different on the remote server, we wrote a script to generate new shellcodes with modified values of target return addresses and to write these values a certain number of times. This program we wrote returned an attack string that we ran using the eval command. After a couple tries changing the variable names, we were able to successfully hack the remote server.

Instructions:

To Compile: g++ -o attack_string_script attack_string_script.cc
To Attack: eval $(./attack_string_script "\x0b\xfe\xff\xbf" 60 500 "310test.cs.duke.edu")
To Connect to the Shell: nc -v 310test.cs.duke.edu 14137

authors: Jason Wang, Yashan Manjunatha, Isaac Andersen
