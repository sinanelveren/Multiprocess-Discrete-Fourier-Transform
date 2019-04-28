# Multiprocess-Discrete-Fourier-Transform

 You are expected to develop a multi-process program consisting of 2 processes; let’s call them
process A and process B.
Process A’s task is to produce endlessly sequences of N random real numbers and write each
sequence into a line of a file X. The file X can contain at most M sequences.
Process B’s task: to read endlessly the sequences in the file X (starting from the last entered line),
and for each sequence:
	a) delete the line where the sequence is located (i.e. replace it with an empty line)
	b) calculate the DFT of the read sequence
	c) print the DFT to the standard output.
You can imagine the file X as a “stack” of size M. Process A tries constantly to fill it with number
sequences, and process B tries constantly to empty it by calculating the DFT of the last entered line



RUN:
	Commands & Ex.Input:
		$make
		$./multiprocess_DFT -N 5 -X file.dat -M 100

NOTE:
	Program can read and write succesfuly in. file.dat. that happens file lock/unlock sussesfuly.
	Program also write/read M count on temp file named info.txt
	DFT has been calculated and print to stdout correctly acording to true sequence (in same line)
	Process A and B 's log files are updating succesfuly.
	Crtl + C is handled and so processes are exiting succesfuly

NOTE2:
	info.txt
		holds line number that 3th number
		and holds firs/last bytes point at last line. Its opsionel for only my trace my code
	file.dat
		holds numbers %09 integer seperated ';'
		there are no end line character. I know where is my end lines.
	*.log
		holds info of processes's action 


