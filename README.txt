The purpose of this program is to design and implement a disassembler for the XE variant of the SIC/XE architecture. Given an object code file and its symbol file, the program should generate a list of its corresponding assembly language counterpart. Despite the previous description of the program, this disassembler is simplified and does not support every possible case. For instance, Format 1 instructions and Constant definitions are not supported.

This README will cover my thought process behind my implementation and design of the disassembler as well as the steps I took to get to the finalized, submitted program. Instead of what could probably be an extremely lengthy discussion on every nuance and case accounted for, I will discuss my design of the more important functionalities of the disassembler that may not be answered to the fullest extent in the program documentation and comments. 


Work Process
------------
Before I dive into explaining my design of the disassembler, I would like to share how I was able to finish a working implementation I am fairly proud of based on my limited knowledge of programming in C++ and disassemblers in general when I first started working on it a couple of weeks ago. This was my first school project in C++; usually, I had been working with other languages such as Java, Python, and most recently C. So, the first thing I did was set up a simple "Hello World!" program and made sure I was able to compile and run that on Visual Studio Code. After I got it working, I turned away from the coding because I wanted to write my thoughts out on notebook and whiteboard. Recording my thoughts somewhere allowed me to keep track of any ideas I had in mind for the design of the program. I also wanted to read up on the textbook and review lecture slides to have a better idea of how to intertwine these multiple functionalities together. I found a wonderful video on YouTube about examples of converting object code into assembly language; that video gave me enough footing to be able to start on at least something. 

Like the video, I translated the object code by hand; not all of it, but just enough for me to get visualize what I'm trying to replicate in the actual program. From there, I eventually started working bit by bit, based on what I had practiced in my notebook. Although it took time to translate what I was doing in my notebook into code, I did want to make sure everything I was taking note of was accounted for, even though I eventually had to account for cases I initially missed while programming.

The progress of my implementation can be divided into multiple periods of time, each with a specific feature (or bug) dedicated to them. The disassembler isn't something where its features can be easily implemented at the same time; one must first create a foundation, that is, getting the acquire the most basic information, in order to move on and build up from there. In between these periods of coding and brainstorming came some rest where I didn't consciouly think about the code or what to do next. I thought it was helpful for me to have a mental reset in order to prevent burnout and a decrease in motivation. I say this ironically enough because for some of the rest periods after a implementation/brainstorm session (I'm looking at you, RESW instructions), I could not stop thinking about the program IF I could not get something to work by the end of that session. With that being said, most of my implementation/brainstorm sessions had me trying to get something to work before I called it a day. Luckily, I believe the longest session I had was implementing the RESW instructions at around 4-6 hours of little-to-no distraction work. Funnily enough, I thought of the solution as I laid in bed and tested it the following morning and it worked. I love it when that happens.

Naturally, I came across multiple bugs and issues with my code that I also spent quite a lot of time figuring out how to fix them. As I was getting closer to finishing my implementation, figuring out what the errors were was easier to pinpoint. By this time, the program was able to finish the disassembling process and provide a list of the assembly language code. It was only a matter of comparing that to the given output list sample before I dived back in and fixed whatever information wasn't correct.

In all, I spent a lot of time working on this project, whether it be writing down ideas/pseudocode, searching on Google about C++ syntax, reading the textbook and lecture slides, or actual coding.


Text Records and Object Code
----------------------------
Although the object code does not distinguish where object codes end, all the necessary information can be obtained by analyzing its three most significant hexadecimals. Instead of converting each individual hexadecimal into an int and playing around bit shifting, I decided to keep it entirely string-based. Patterns in the hexadecimal digits when looking at their individual bits exist to the point where I was able to find patterns of which hexadecimal digits meant what for different pieces of information: Opcode, Mnemonic, Format, Addressing Mode for the Target Address, and Addressing Mode for the Operand Value. In terms of where an object code ends, the format is enough to determine how long the object code is. As such, I extract the three hexadecimals individually and match them to test for specific conditions within a certain piece of information. I also utilized the given mnemonic and opcode string data structures so it was easier for me to keep this portion of the code using strings rather than integers. 


Parsing the Symbol File
-----------------------
Parsing the symbol file took me longer than I expected due to its somewhat unfriendly structure compared to the easily parsed object file. After a fair amount of testing and trying to figure out what the best way to store the required information, I eventually ended using a map to store it with the keys being the addresses and the values being the name of the labels and literals themselves. I used tokens to parse specific information and had to implement different cases to ensure I was only obtaining and storing the addresses and the labels/literals themselves. The rest of the information in the symbol file was irrelevant in respect to the function of the disassembler.


Disassembler Outline
--------------------
If I could broadly recreate my disassemble function, it would look like this:

void disassemble(objectFile, symbolFile){
	parse object file, store in vector
	parse symbol file, store in map
	store initial information from records
	for(each text record){
		find and store starting address
		for(starting char in object code section until it gets to the end of the text record){
			get the necessary information (program counter, labels, opcode, operand, object code)
			account for extra cases
			store the necessary information
			update program counter and where what char to start from for next iteration
		}
		fill any address gaps between text records if applicable
	}
	store information from end record
	create output based on stored information
}

I decided to make global vectors for storing the program counter addresses, labels, opcodes, operands, and object codes. As such, I wanted to make sure I extracted every piece of information for each line and simply output everything in the end line by line. I did think going into this implementation design that this was a risky option and required stricter testing in order to ensure that each global vector was the same size by the end of the disassembling process so there wouldn't be any output errors. However, I did not want to intermix creating an output file with the disassembling process just for organization's sake, so I soldiered on with the global vector design. 

The extra nuances and cases are documented in the source code so I won't discuss it any further here; I just want to provide a gist of how I implemented the disassembling process without making it as difficult to look at. However, I will add that the extra nuances and cases that were building up during implementation significantly impacted the order of executed code. When I say impacted, I mean that the order of my actual code is not as neatly organized as the pseudocode above. Depending on the specific case, certain lines of code had to go at the beginning of the iteration through the object code rather than after all the important information for the object code was acquired. Cases such as LTORG instructions and literal definitions forced me to go back to the drawing board to figure out how to effectively structure program instructions to ensure that all vectors would be the same size with each index representing a line of assembly language instruction. 


LTORG and Literal Instructions
------------------------------
Because most of my documentation regarding LTORG and literals are just comments, I will briefly discuss my thought process here. Essentially, when the current program counter address matches that of a literal, the LTORG instruction must be added before the declaration of the literals that were referenced prior. This is why checking for this condition is at the beginning of the disassembling inner loop (lines 91-161 in disassembler.cpp). Within this condition comes another condition that while the current program counter address matches that of where a literal is located, create a line of instruction declaring it. This is to account for multiple literals that were referenced in earlier lines of instruction. If there was only one literal, the updated program counter address would no longer match that of a literal's location and the LTORG and Literal process would end and continue on in the text records. 


RESW Instructions
-----------------
Getting RESW instructions to work properly was certainly my biggest challenge as I spent the most time on figuring out the best way to implement it. My first idea was so convoluted and complex that I forgot what I actually did; it was that bad. However, I was eventually able to get what I think is the most efficient way possible to fill in these address gaps between text records and the overall length of the program. Although I explain it some detail in the source code documentation, I will also explain it here for convenience. 

I wanted to create a vector of addresses with this range:

[Ending program counter address of the current text record being analyzed, Starting program counter address of the next text record to be analyzed]

This is for filling the gaps between text records ONLY. For the last text record, I would instead have this range:

[Ending program counter of the last text record being analyzed, Entire length of program (found in header record) ]

Along with the min and max range of addresses are the addresses of where symbols are located based on the symbol table. The symbol addresses that were in those ranges were added to the vector. Since the vector would be a list of addresses in ascending order, the number of bytes to reserve between them would simply be (vector[i + 1] - vector[i]) / 3, where i is the current iteration through the vector, excluding the last address, which would be the starting program counter address of the next text record or the length of the program. 

This rather quick explanation makes me wonder what I was thinking prior to this; it was kind of bittersweet figuring out an easy and working solution AFTER working so long on making something work. I guess your best ideas come to you after you're trying to not think about it.


 





			

















