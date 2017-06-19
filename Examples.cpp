#include "tiss.h"
#include <stdio.h>


struct Com {

	Com(int a) { }
	Com() { }

	int operator()(int &x) {
		printf("functor ");
		printf("x: %d -> %d\n", x, x + 1);
		x += 1;
		return 1;
	}

	int foo(int &x) {
		printf("member function! ");
		printf("x: %d -> %d\n", x, x + 1);
		x += 1;
		return 1;
	}

	int bar(int &x) const {
		printf("member const function! ");
		printf("x: %d -> %d\n", x, x + 1);
		x += 1;
		return 1;
	}
};


void example_connect()
{
	using namespace std::placeholders;
	tiss::signal<int(int&)> s;

	// the functor will be moved/copied to the internal space
	s.connect([&](int &x) {
		printf("lambda: ");
		printf("x: %d -> %d\n", x, x + 1);
		x += 1;

		return 1;
	});

	// equivalent to s.connect(std::bind([&](int const &, int &x) { ... }, 1, std::placeholders::_1))
	// but the return results of std::bind will constructed inplace the connection body
	// so move/copy constructor in not needed in this version
	s.connect_bind([&](int const &, int &x) {
		printf("binder: ");
		printf("x: %d -> %d\n", x, x + 1);
		x += 1;

		// same as s.connect(Com(1))
		// but Com will constructed inplace the connection body
		// so move/copy constructor in not needed in this version

		// connecting a new slot, while another slot is running, is OK.
		s.connect_emplace<Com>(1);

		return 1;
	}, 1, std::placeholders::_1);

	int(*ptr)(int &) = [](int &x) {
		printf("free function: ");
		printf("x: %d -> %d\n", x, x + 1);
		x += 1;

		return 1;
	};
	s.connect_funcptr(ptr);
	Com com;
	Com const constCom;
	s.connect_funcptr(&com, &Com::foo);
	s.connect_funcptr(&com, &Com::bar);
	s.connect_funcptr(&constCom, &Com::bar);
	// s.connect_funcptr(&constCom, &Com::foo);

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
	printf("num of connections %d\n", (int)s.num_connections());
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
	s.emit_and_get_last_result(a, b);
	printf("last result %d\n", b);

	int i = 0;
	s(a, [&](int b) {
		printf("%dth result %d\n", i, b);
		++i;
	});

	i = 0;
	for (auto b : s.emit_and_get_range(a)) {
		printf("%dth result %d\n", i, b);
		++i;
	}

}


void example_emit_util_false() {
	tiss::signal<bool()> s;
	s.connect([]() -> int {
		printf("this is connection 1, return 1\n");
		return 1;
	});
	s.connect([]() {
		printf("this is connection 2, return false\n");
		return false;
	});
	s.connect([]() {
		printf("this is connection 3, return true\n");
		return true;
	});
	printf("example_emit_util_false\n");
	s.emit_util_false();
	printf("example_emit_util_true\n");
	s.emit_util_true();
}

struct Object {
	Object() { printf("default cotr\n"); }
	Object(Object const &o) { printf("copy cotr\n"); }
	Object(Object &&o) { printf("move cotr\n"); }
	Object& operator=(Object const &o) { printf("copy =\n"); }
	Object& operator=(Object &&o) { printf("move =\n"); }
	~Object() { printf("~\n"); }
};


void example_safe_forward()
{
	printf("example_safe_forward\n");
	tiss::signal<int(std::string)> s;

	s.connect([&](std::string str) {
		printf("connection1, %s\n", str.c_str());
		return 0;
	});
	s.connect([&](std::string str) {
		printf("connection2, %s\n", str.c_str());
		return 0;
	});

	s("abc");
}

void example_safe_forward2()
{
	printf("example_safe_forward2\n");
	tiss::signal<int(Object)> s;

	s.connect([&](Object obj) {
		printf("this is connection1\n");
		return 0;
	});
	s.connect([&](Object const &obj) {
		printf("this is connection2\n");
		return 0;
	});
	s.connect([&](Object obj) {
		printf("this is connection3\n");
		return 0;
	});
	s(Object());
}

int main() {
	example_connect();
	example_disconnect();
	example_get_result();
	example_emit_util_false();
	example_safe_forward();
	example_safe_forward2();
	static_assert(std::is_same<tiss::details::copy_forward_type<int&>, int &>::value, "");
	static_assert(std::is_same<tiss::details::copy_forward_type<int>, int const &>::value, "");
	static_assert(std::is_same<tiss::details::copy_forward_type<int &&>, int &&>::value, "");
	static_assert(std::is_lvalue_reference<int const &>::value, "");

}

