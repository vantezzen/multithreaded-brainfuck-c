This program uses 3 threads that will each output a character

Create two forks
/######/

Move main thread some cells to the right
++>++>
#######

This runs on main thread and first fork
>++>

This runs on all forks
>++>

The threads are now on cells:
Main: 6
First child: 4
Third child: 2

Add 65 to the current cell in each thread so we can output characters
<++++++[->++++++++++<]>
+++++

Output the current number
.
