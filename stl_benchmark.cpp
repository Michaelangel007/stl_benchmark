/*

= Compile =

== Windows ==

	Use Microsoft Visual Studio 11 or better

== Linux/OSX ==


	Default	Timer

        gcc -lstdc++ -O2 stl_benchmark.cpp -o stl_benchmark

	Specific Timers

        gcc -DTIMER=Timer1 -lstdc++ -O2 stl_benchmark.cpp -o stl_benchmark_high
        gcc -DTIMER=Timer2 -lstdc++ -O2 stl_benchmark.cpp -o stl_benchmark_chrono
        gcc -DTIMER=Timer3 -lstdc++ -O2 stl_benchmark.cpp -o stl_benchmark_clock
*/


#include <iostream>

#include <list>
#include <map>
#include <unordered_map>
#include <vector>

#include <chrono>

// http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system
#ifdef WIN32
	#include <Windows.h>
#elif __APPLE__ // OSX
	// https://developer.apple.com/library/mac/qa/qa1398/_index.html
	// http://stackoverflow.com/questions/464618/whats-the-equivalent-of-windows-queryperformancecounter-on-osx
	#include <CoreServices/CoreServices.h>
	#include <mach/mach.h>
	#include <mach/mach_time.h>
	#include <unistd.h>
#elif __linux__ // LINUX
	// http://stackoverflow.com/questions/538609/high-resolution-timer-with-c-and-linux
	// https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_MRG/2/html/Realtime_Reference_Guide/Realtime_Reference_Guide-Timestamping-Clock_Resolution.html
	// http://tdistler.com/2010/06/27/high-performance-timing-on-linux-windows
	// http://www.guyrutenberg.com/2007/09/22/profiling-code-using-clock_gettime/
	#include <sys/time.h>
#endif

//#include <sstream> // not needed

using namespace std;

#ifndef TIMER
	//#define TIMER Timer1 // High Performance Counter
	  #define TIMER Timer2 // C++ chrono
	//#define TIMER Timer3 // clock
#endif

class Timer1
{
public:
#ifdef WIN32
	Timer1()
	{
		QueryPerformanceFrequency(&frequency);
		start.QuadPart = 0;
		end.QuadPart   = 0;
	}

	void Start()
	{
		start.QuadPart = 0;
		QueryPerformanceCounter(&start);
	}

	void Stop()
	{
		QueryPerformanceCounter(&end);
		elapsed.QuadPart = end.QuadPart - start.QuadPart;
	}

	double GetTicks()
	{
		return elapsed.QuadPart;
	}

	double GetMs()
	{
		double diff = (end.QuadPart - start.QuadPart) ;
		double tick_interval = 1000.0 / frequency.QuadPart;
		return diff * tick_interval;
	}

private:
	LARGE_INTEGER start, end, elapsed;
	LARGE_INTEGER frequency;

#elif __APPLE__

	Timer1()
	{
		cout <<	"Timer: OSX High Precision\n";
	}

	void Start()
	{
		start = mach_absolute_time();
	}

	void Stop()
	{
		end     = mach_absolute_time(); // nanoseconds
		elapsed = end - start;
	}

	double GetTicks()
	{
		return elapsed;
	}

	double GetMs()
	{
		return elapsed / 1000000LL;
	}

private:
	uint64_t start, end, elapsed;

#elif __linux__
	Timer1()
	{
		timespec result;
		clock_getres( CLOCK_MONOTONIC, &result );
		frequency  = result.tv_nsec;
		cout << "Timer: Linux High Precision   Frequency: " << frequency << "ns \n";
	}

	void Start()
	{
		clock_gettime( CLOCK_MONOTONIC, &start );
	}

	void Stop()
	{
		clock_gettime( CLOCK_MONOTONIC, &end );
		elapsed = end.tv_nsec - start.tv_nsec;
	}

	double GetTicks() const
	{
		return elapsed;
	}

	double GetMs() const
	{
		// http://stackoverflow.com/questions/1269994/nanoseconds-to-milliseconds-fast-division-by-1000000
		// You will want to verify your compiler actually optimizes this division into a mul
		return elapsed / 1000000UL;
	}

private:
	timespec start;
	timespec end;
	uint64_t elapsed;

#endif // WIN32, APPLE, LINUX
};

class Timer2
{
public:
	Timer2()
	{
		cout << "Timer: std::chrono  Precision:" << hrclock_t::period::den << endl;
	}

	void Start()
	{
		start = hrclock_t::now();
	}

	void Stop(){
		end = hrclock_t::now();
		elapsed = (end - start);
	}

	double GetMs() const
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	}

	double GetTicks() const{
		return elapsed.count();
	}

private:
	typedef std::chrono::high_resolution_clock hrclock_t;
	std::chrono::time_point<hrclock_t> start;
	std::chrono::time_point<hrclock_t> end;
	std::chrono::duration<double>      elapsed;
};

class Timer3{
public:
	Timer3()
	{
		cout << "Timer: clock()   Precision: Clocks/Second: " << CLOCKS_PER_SEC << endl;
	}

	void Start()
	{
		start = clock();
	}
	
	void Stop()
	{
		end = clock();
		elapsed = end - start;
	}

	double GetMs() const
	{
		double timeInSeconds = elapsed / (double)CLOCKS_PER_SEC;
		return timeInSeconds * 1000.;
	}

	double GetTicks() const
	{
		return elapsed;
	}
private:
	clock_t start;
	clock_t end;
	clock_t elapsed; // clock ticks
};

class A
{
public:
	A(int x)
	{
		c = x;
#if 0
		std::stringstream ss;
		ss << x;
		string = ss.str();
#endif
		string = std::to_string( x );
	}

	std::string GetString()
	{
		return string;
	}
private:
	int c;
	std::string string;
};


class Foo
{
public:
	Foo(int x)
	{
		this->x = x;
		a1 = new A(x);
		a2 = NULL;		
	}

	~Foo(){
		delete a1;
	}

	int Get() const{
		return x;
	}

	std::string GetString()
	{
		return a1->GetString();
	}
private:
	int x;
	A*  a1;
	A*  a2;
};

void print(const char* type, TIMER&	timer )
{
	const double time  = timer.GetMs();
    const double ticks = timer.GetTicks();

		cout << "\n*****\n" << type  << endl;
		cout << "ms: "      << time  << endl;
	if (ticks != 0)
		cout << "ticks: "   << ticks << endl;
}

int main()
{
	TIMER timer;

	int n      = 1000000;
	int total  = 0;
	int actual = 0;

	std::map<          int, Foo*> _map;
	std::unordered_map<int, Foo*> _unordered;
	std::vector<            Foo*> _vector;
	std::list<              Foo*> _list;
	Foo** array = (Foo**)malloc(n * sizeof(Foo));

	//allocate
	cout << "Allocating elements\n";
	timer.Start();
		for (size_t i = 0; i < n; ++i)
		{
			_map      [i]    = new Foo(i);
			_unordered[i]    = new Foo(i);
			_list.push_back(   new Foo(i));
			_vector.push_back( new Foo(i));
			array     [i]    = new Foo(i);
		}
	timer.Stop();
	cout << "Number of elements: " << n << "  ms: " << (int) timer.GetMs() << endl;


	//STL map
	timer.Start();
		for (const auto& i : _map)
		{
			total += i.second->Get();
			std::string str = i.second->GetString();
		}
	timer.Stop();
	print("stl map", timer );
	actual = total;

	// STL unordered map
	total = 0;
	timer.Start();
		for (const auto& i : _unordered)
		{
			total += i.second->Get();
			std::string str = i.second->GetString();
		
		}
	timer.Stop();
	print("stl unordered_map", timer );
	if (total != actual)
		cout << "Total: " << total << endl;

	//STL vector
	total = 0;
	timer.Start();
		for (int i = 0; i < n; ++i)
		{
			total += _vector[i]->Get();
			std::string str = _vector[i]->GetString();
		
		}
	timer.Stop();
	print("stl vector", timer );
	if (total != actual)
		cout << "Total: " << total << endl;

	//STL list
	total = 0;
	timer.Start();
		for (Foo* i: _list)
		{
			total += i->Get();
			std::string str = i->GetString();
		}
	timer.Stop();
	print("stl list", timer );
	if (total != actual)
		cout << "Total: " << total << endl;

	//array
	total = 0;
	timer.Start();
		for (int i = 0 ; i < n; ++i)
		{
			total += array[i]->Get();
			std::string str = array[i]->GetString();
		}
	timer.Stop();
	print("array", timer );
	if (total != actual)
		cout << "Total: " << total << endl;

	free(array);

	//getchar();
	return 0;
}
