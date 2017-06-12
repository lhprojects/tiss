#include "tiss.h"


struct Com {

	Com(int a) { }
	int operator()(int &x) {
		printf("x: %d -> %d\n", x, x + 1);
		x += 1;
		return 1;
	}
};


void example_connect()
{
	using namespace std::placeholders;
	tiss::signal<int(int&)> s;


	// so move/copy constructor in needed in this version
	// only one move/coy construction will be perform
	s.connect([&](int &x) {

		printf("x: %d -> %d\n", x, x + 1);
		x += 1;

		// equivalent to s.connect(std::bind([&](int const &, int &x) { ... }, 1, std::placeholders::_1))
		// but the return results of std::bind will constructed inplace the connection body
		// so move/copy constructor in not needed in this version
		s.connect_bind([&](int const &, int &x) {
			printf("x: %d -> %d\n", x, x + 1);
			x += 1;

			// same as s.connect(Com(1))
			// but Com will constructed inplace the connection body
			// so move/copy constructor in not needed in this version
			s.connect_emplace<Com>(1);

			return 1;
		}, 1, std::placeholders::_1);


		return 1;
	});

	int a = 0;
	s(a);

}

void example_disconnect()
{
	tiss::signal<int(int&)> s;
	int a;
	tiss::connection con1;
	tiss::connection con2;
	con1 = s.connect([&](int &) {
		printf("this is connection 1\n");
		printf("we will do disconnection\n");
		// disconnect myself
		// the disconnection will perform after this call
		con1.disconnect();
		// disconnect next connection
		// the disconnection will perform immediately
		con2.disconnect();
		return 0;
	});
	con2 = s.connect([&](int &) {
		printf("this is connection 2\n");
		// disconnect myself
		// the disconnection will perform after this call
		return 0;
	});
	s(a);
	printf("the first  connection status %d\n", con1.connected());
	printf("the second connection status %d\n", con2.connected());
	s(a);
}

void example_get_result()
{
	tiss::signal<int(int&)> s;
	int a;
	s.connect([&](int &) {
		printf("this is connection 1\n");
		printf("we will return 1\n");
		return 1;
	});
	
	s.connect([&](int &) {
		printf("this is connection 2\n");
		printf("we will return 2\n");
		return 2;
	});

	// result will be droped
	s(a);

	int b;
	s.invoke_and_get_last_result(a, b);
	printf("last result %d\n", b);

	int i = 0;
	s(a, [&](int b) {
		printf("%dth result %d\n", i, b);
		++i;
	});

}

int main() {
	example_connect();
	example_disconnect();
	example_get_result();

}

