Project 2: Hack Into a Server
Authors: Jason Wang, Yashas Manjunatha, Isaac Andersen

Instructions to Hack the Remote Server:

To Compile: g++ -o attack_string_script attack_string_script.cc
To Attack (The Shell Should Hang): eval $(./attack_string_script "\x0b\xfe\xff\xbf" 60 500 "310test.cs.duke.edu")
To Connect to the Shell (In a New Terminal): nc -v 310test.cs.duke.edu 14137
To Modify index.html (In the New Terminal): cd www echo "[Insert String to Append]" >> index.html

Explanation of Method of Attack:

First, we identified the vulnerability in webserver.c, which happens in the check_filename_length method. The function's parameter len, which represents the length of the filename/attack string, is of type byte, which has a value range of 0 to 255. So the method which checks that len < 100 would evaluate true if len is in the range 256 - 355 or 512 - 611 or 768 - 867 etc. Thus, for our buffer overflow attack we need to ensure that the length of the attack string falls in this range.

Next, we constructed our attack string. The approach is as follows: First, the attack string will comprise of the desired return address repeated many times. This is to ensure that the attack string overflows the buffer (the filename array) and overwrites the old return address with our return address. Following the series of return addresses, we append a nop sled of considerable length to increase the chance that our return address will land in the nop sled and eventually execute our shellcode. Finally, we end the attack string with a modified version of the suggested Shellcode 357 (http://shell-storm.org/shellcode/files/shellcode-357.php). We modified the port from the default number 5074 to our chosen port 14137. Of course the attack string is prefixed by "echo -e "GET /" and suffixed by " HTTP" | nc 127.0.0.1 9422". The "127.0.0.1" is the local server, and is replaced with "310test.cs.duke.edu" when hacking the remote server.

Note: Return addresses (ex. \xob\xfe\xff\xbf) are 4 characters = 4 bytes long. A nop (0x90) is 1 byte long. The modified shell code is 92 bytes in length.

To hack a local copy of the webserver, we constructed a stack-smash attack using the approach described above. First, we constructed an attack string using the return address (\xcc\xcf\xff\xff) repeated 60 times. This return address was chosen by adding 128 (randomly chosen) to the gdb output value of addr+140 as a reasonable guess of where we could enter the nop sled. Then, appended a nop sled with 500 nop instructions followed by the modified shellcode. The length of the attack string was 60 * 4 + 500 * 1 + 92 = 832, which is in the acceptable length range for the attack string as discussed earlier. This successfully hacked the local server, and we were able to modify index.html after connecting to the shell on the new port.

After performing a successful hack on the local server and acknowledging that the base stack address would be slightly different on the remote server, we wrote a script (attack_string_script.cc) to generate new attack strings with modified values of target return addresses and to write these values a certain number of times. This script takes in the following parameters, in order: string representing the new target return address, the number of times to repeat the return address, the number of nop instructions in the nop sled, a string representing the server ("127.0.0.1" for local server and "310test.cs.duke.edu" for the remote server). This program we wrote returned an attack string that we ran using the eval command. After a couple tries changing the target return address variable, we were able to successfully hack the remote server. The length of this final attack string was also 60 * 4 + 500 * 1 + 92 = 832, which passed the filename length check in webserver.c. We ran the command "echo "please oh dear god help me finish this thank you" >> index.html" to modify the file on the remote server to prove we successfully hacked the server.

Review of Project:

As a whole, this project was really fun to work on. We learned a lot more about what "hacking" really means and got a better understanding of assembly, stack memory, and how to find exploits. Furthermore, getting to hack into a remote server was just a really cool thing to accomplish.
