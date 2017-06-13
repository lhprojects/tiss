#include <boost/signals2.hpp>
#include <chrono>
#include <iostream>
#include "tiss.h"

// baseline
// 1ms / 10000000  =  0.25*(1mul+2move+1sub+jmp)

volatile int x;
void foo(int i_, int &a) {
	for (int i = 0; i < 1; ++i) {
		a = i_*x;
	}
}

void test_invoke()
{
	printf("test_invoke\n");
	namespace cr = std::chrono;

	{
		auto t0 = cr::high_resolution_clock::now();

		boost::signals2::signal<void(int, int&)> signal;
		signal.connect(foo);
		auto sum = 0;
		for (int i = 0; i < 10000000; ++i) {
			int a;
			signal(i, a);
			sum += a;
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}
	{
		auto t0 = cr::high_resolution_clock::now();

		tiss::signal<void(int, int&)> signal;
		signal.connect(foo);
		auto sum = 0;
		for (int i = 0; i < 10000000; ++i) {
			int a;
			signal(i, a);
			sum += a;
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}
	{
		auto t0 = cr::high_resolution_clock::now();

		auto sum = 0;
		for (int i = 0; i < 10000000; ++i) {
			int a;
			foo(i, a);
			sum += a;
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}

}

void test_invoke2()
{

	printf("test_invoke2\n");
	namespace cr = std::chrono;

	{
		printf("boost.signal2\n");
		auto t0 = cr::high_resolution_clock::now();

		boost::signals2::signal<void(int, int&)> signal;
		signal.connect(foo);
		signal.connect(foo);
		auto sum = 0;
		for (int i = 0; i < 10000000; ++i) {
			int a;
			signal(i, a);
			sum += a;
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}
	{
		printf("tiss.signal\n");
		auto t0 = cr::high_resolution_clock::now();

		tiss::signal<void(int, int&)> signal;
		signal.connect(foo);
		signal.connect(foo);
		auto sum = 0;
		for (int i = 0; i < 10000000; ++i) {
			int a;
			signal(i, a);
			sum += a;
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}

	{
		printf("tiss.signal.invoke_and_get_range\n");
		auto t0 = cr::high_resolution_clock::now();

		tiss::signal<void(int, int&)> signal;
		signal.connect(foo);
		signal.connect(foo);
		auto sum = 0;
		for (int i = 0; i < 10000000; ++i) {
			int a;
			auto rng = signal.invoke_and_get_range(i, a);
			tiss::_Get_Tuple<void(int, int&)>::type;
			auto b = rng.begin();
			auto e = rng.end();
			for (; b != e; ++b) {
			}
			sum += a;
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}

	printf("raw\n");
	{
		auto t0 = cr::high_resolution_clock::now();

		auto sum = 0;
		for (int i = 0; i < 10000000; ++i) {
			int a;
			foo(i, a);
			foo(i, a);
			sum += a;
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}

}

void test_invoke10()
{

	printf("test_invoke10\n");
	namespace cr = std::chrono;

	{
		printf("boost.signal2\n");
		auto t0 = cr::high_resolution_clock::now();

		boost::signals2::signal<void(int, int&)> signal;
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		auto sum = 0;
		for (int i = 0; i < 10000000; ++i) {
			int a;
			signal(i, a);
			sum += a;
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}
	{
		printf("tiss.signal\n");
		auto t0 = cr::high_resolution_clock::now();

		tiss::signal<void(int, int&)> signal;
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		auto sum = 0;
		for (int i = 0; i < 10000000; ++i) {
			int a;
			signal(i, a);
			sum += a;
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}

	{
		printf("tiss.signal.invoke_and_get_range\n");
		auto t0 = cr::high_resolution_clock::now();

		tiss::signal<void(int, int&)> signal;
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		signal.connect(foo);
		auto sum = 0;
		for (int i = 0; i < 10000000; ++i) {
			int a;
			auto rng = signal.invoke_and_get_range(i, a);
			tiss::_Get_Tuple<void(int, int&)>::type;
			auto b = rng.begin();
			auto e = rng.end();
			for (; b != e; ++b) {
			}
			sum += a;
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}

	printf("raw\n");
	{
		auto t0 = cr::high_resolution_clock::now();

		auto sum = 0;
		for (int i = 0; i < 10000000; ++i) {
			int a;
			foo(i, a);
			foo(i, a);
			foo(i, a);
			foo(i, a);
			foo(i, a);
			foo(i, a);
			foo(i, a);
			foo(i, a);
			foo(i, a);
			foo(i, a);
			sum += a;
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}

}

void test_heavy_invoke()
{

	printf("test_heavy_invoke\n");
	namespace cr = std::chrono;

	{
		auto t0 = cr::high_resolution_clock::now();

		boost::signals2::signal<std::string(std::string const &str)> signal;
		signal.connect([](std::string const &str) { return str; });
		std::string str = "123456789012345678901234567890";

		for (int i = 0; i < 10000000; ++i) {
			signal(str);
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}
	{
		auto t0 = cr::high_resolution_clock::now();

		tiss::signal<std::string(std::string const &str)> signal;
		signal.connect([](std::string const &str) { return str; });
		std::string str = "123456789012345678901234567890";
		for (int i = 0; i < 10000000; ++i) {
			signal(str);
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}
	{
		auto t0 = cr::high_resolution_clock::now();

		tiss::signal<std::string(std::string const &str)> signal;
		signal.connect([](std::string const &str) { return str; });
		std::string str = "123456789012345678901234567890";
		for (int i = 0; i < 10000000; ++i) {
			for(auto &&str : signal.invoke_and_get_range(str)) { }
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}
	{
		auto t0 = cr::high_resolution_clock::now();

		std::string str = "123456789012345678901234567890";
		auto copy_str = [](std::string const &str) { return str; };
		for (int i = 0; i < 10000000; ++i) {
			copy_str(str);
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}

}

void test_connect()
{

	printf("test_connect\n");

	namespace cr = std::chrono;

	{
		auto t0 = cr::high_resolution_clock::now();

		boost::signals2::signal<void(int, int&)> signal;
		for (int i = 0; i < 10000000; ++i) {
			signal.connect(foo);
			signal.disconnect_all_slots();
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}
	{
		auto t0 = cr::high_resolution_clock::now();

		tiss::signal<void(int, int&)> signal;
		for (int i = 0; i < 10000000; ++i) {
			signal.connect(foo);
			signal.disconnect_all();
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}

}

void test_heavy_lambda_connect()
{

	printf("test_heavy_lambda_connect\n");

	namespace cr = std::chrono;

	{
		auto t0 = cr::high_resolution_clock::now();

		boost::signals2::signal<void()> signal;
		std::string str = "123456789012345678901234567890";
		for (int i = 0; i < 10000000; ++i) {
			signal.connect([str]() {  int i = x; });
			signal.disconnect_all_slots();
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}
	{
		auto t0 = cr::high_resolution_clock::now();

		tiss::signal<void()> signal;
		std::string str = "123456789012345678901234567890";
		for (int i = 0; i < 10000000; ++i) {
			signal.connect([str]() { int i = x; });
			signal.disconnect_all();		
		}

		auto t1 = cr::high_resolution_clock::now();
		std::cout << cr::duration_cast<cr::milliseconds>(t1 - t0).count() << std::endl;
	}

}

int main()
{
	test_invoke();
	test_invoke2();
	test_invoke10();
	test_heavy_invoke();
	test_connect();
	test_heavy_lambda_connect();
	return 0;
}
