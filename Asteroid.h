#ifndef PROFILER
#define PROFILER

#include <chrono>

#define PROFILER_INIT() ProfilerInit();

#define PROFILER_CLEANUP() ProfilerCleanup();

#define PROFILER_START_FRAME() ProfilerFrameData profilerFrameData;

#define PROFILER_FUNCTION() ProfilerFunctionData profilerFunctionData(__FUNCTION__, __FILE__, __LINE__, sizeof(__FUNCTION__), sizeof(__FILE__));

void ProfilerInit();
void ProfilerCleanup();

class Clock {
public:
	Clock() {}

	void Start()
	{
		start = std::chrono::high_resolution_clock::now();
	}

	double ReadMs()
	{
		return  std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
	}

private:

	std::chrono::steady_clock::time_point start;
};

class ProfilerFrameData {
public:
	ProfilerFrameData();
	~ProfilerFrameData();

private:

	Clock clock;
};

class ProfilerFunctionData {
public:
	ProfilerFunctionData(const char* functionName, const char* fileName, int line, int functionSize, int fileSize);
	~ProfilerFunctionData();

private:

	Clock clock;
};

#endif