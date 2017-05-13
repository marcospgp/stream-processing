# stream-processing
 Operating systems project of year 16/17. Made with @sergiotj97 and @vitorindeep

## What does it do?

 This collection of tools allows the user to create a network of nodes that parse lines of input from the system. Its functionality is similar to Apache's Storm.
 
 ## How to use?
 
 #### Step 1:
 Set WINDOWS_MODE to 0 or 1 in globals.h depending on your operating system

 #### Step 2:
 Run `make`
 
 #### Step 3:
 Run `controller config.txt` (or `controller.exe config.txt` on windows)

 #### Step 4:
 Enter "stop" in the terminal in order to close all processes and delete all FIFO pipe files.
