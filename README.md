# Multi-thread-word-search
One of the university projects we had as homework.
Original project text is in the "OS Domaci 4" file.

The program can search for words while simultaneously getting imput from the user.
The user can write a prefix that they want to be searched for and will slowly get new words poped on the screen
if the program finds additional words in the meantime (this shows the multi-thread work).

The program was tested on Linux ubuntu. Start the program from the console and you can write "_ add _ <folder_name>"(just remove spacebars between _ and add)
to search the provided folder, the files inside will be searched for the word provided. The words in the file
must be separated with spacebars.
To stop the ongoing search use the combination CTRL+D, now you can either write "_ add _ <folder_name>"(just remove spacebars between _ and add) to add another folder
or "_ stop _"(just remove spacebars between _ and stop) to exit the program.

The program could have potential rare bugs because some variables werent protected from simultaneous access from different
threads. This was noticed after the program was finished and I was just lazy to fix it :P

This was my first encounter with multi-thread programming.
Hope you have fun with this one :)
