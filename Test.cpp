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



struct Com {

	Com(int a) { }
	int operator()(int &x) {
		printf("%d -> %d\n", x, x+1);
		x += 1;
		return 1;
	}
};
int main()
{

	using namespace std::placeholders;
	tiss::signal<int(int&)> s;


	s.connect([&](int &x) { 
		printf("%d -> %d\n", x, x + 1);
		x += 1;

		s.connect_bind([&](int const &, int &x) {
			printf("%d -> %d\n", x, x + 1);
			x += 1;

			s.connect_emplace<Com>(1);

			return 1;
		}, 1, std::placeholders::_1);


		return 1;
	});

	int a = 0;
	s(a);
	s.disconnect_all();
	tiss::connection con;


	con = s.connect([&] (int &) {
		printf("disconnect\n");
		con.disconnect();
		return 0;
	});
	s(a);
	s(a);

	test_invoke();
	test_invoke2();
	test_heavy_invoke();
	test_connect();
	test_heavy_lambda_connect();

	return 0;
}
