#pragma once

// Class to benchmark the average execution time of multiple code executions and write it to standard out.
// Can be used to meassure the execution time of a function hook for gui apps with an allocated console.

namespace hax {
	
	class Bench {
	private:
		double _startTime;
		double _endTime;
		const char* const _label;
		double* _durations;
		size_t _counter;
		const size_t _runs;

	public:
		// Initializes members
		// Parameters:
		// [in] label:	Label for the output. Format: "Average time <label>: <average time in seconds>"
		// [in] runs:	How many times the code execution is meassured before the average is calculated and printed when calling printAvg.
		Bench(const char* label, size_t runs);
		
		~Bench();
		
		// Starts the meassurement.
		void start();
		
		// Ends the meassurement.
		void end();

		// Writes the output to std out after the code inbetween start() and end() is executed the amount of times passed as runs to the constructor.
		void printAvg();
	};

}