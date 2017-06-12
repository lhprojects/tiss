#ifndef TISS_H
#define TISS_H

#include <utility>
#include <memory>
#include <type_traits>
#include <functional>

namespace tiss {

	namespace details {

		struct linked {

			linked() {
				fPrev = this;
				fNext = this;
			}
			linked *fPrev;
			linked *fNext;

			// if we are list header
			void push_back(linked *p) {
				p->fNext = this;
				p->fPrev = fPrev;
				fPrev->fNext = p;
				fPrev = p;
			}

			bool empty() { return fPrev == this; }
		};
	}

	class connection_body_vptr
	{
	public:
		// memory layout
		// vptr

		connection_body_vptr() { }
		connection_body_vptr(connection_body_vptr const &r) = delete;
		connection_body_vptr(connection_body_vptr &&r) = delete;
		connection_body_vptr &operator=(connection_body_vptr const &r) = delete;
		connection_body_vptr &operator=(connection_body_vptr &&r) = delete;

		virtual void Destroy() = 0;


	};

	struct linked_connection_body_base : public connection_body_vptr, public details::linked
	{
		// memory layout
		// vptr
		// fPrev
		// fNext
		// fWeakRef
		// fStrongRef
		// fConnected

		size_t fWeakRef;
		size_t fStrongRef;
		bool fConnected = true;


		// we use linked as base class
		// we wanna static_cast<linked_connection_body_base&>(node);
		// we don't have to calcuate offset by myself

		linked_connection_body_base()
		{
			fWeakRef = 1;
			fStrongRef = 1;
		}

		void RemoveFromList()
		{
			fNext->fPrev = fPrev; // removed this from the list
			fPrev->fNext = fNext;
		}

		void Disconnect()
		{
			if (fConnected) {
				fConnected = false;
				DecStrongRef();  // let signal give up the strong ref
			}
		}

		void IncStrongRef() {
			fStrongRef++;
		}

		void IncWeakRef() {
			fWeakRef++;
		}

		void DecStrongRef() {
			fStrongRef--;
			if (fStrongRef == 0) {
				// just image there is weak ref if fStrongRef > 0
				RemoveFromList();
				Destroy();
				DecWeakRef();
			}
		}

		void DecWeakRef() {
			fWeakRef--;
			if (fWeakRef == 0) {
				// no strong ref and no weak ref
				DeleteThis();
			}
		}

		void DeleteThis() {
			delete this; // just free memory
			             // because the derived class deconstructor will do nothing
			             // Resource will be destroy by Desctroy
		}
	};

	template<class Return, class... Args>
	class connection_body : public linked_connection_body_base
	{

	public:
		using connection_body_type = connection_body<connection_body<Return, Args...> >;

		virtual Return Invoke(Args&&... args) = 0;

	};

	template<class FuncStorage, class Return, class... Args>
	class connection_body_derived final : public connection_body<Return, Args...> {
	public:
		// memory layout
		// vptr
		// fPrev
		// fNext
		// fWeakRef
		// fStrongRef
		// fConnected
		// fFuncStore

		using connection_body_type = connection_body<Return, Args...>;

		union {// forbidden default constructor and deconstructor
			FuncStorage fFuncStore;
		};

		connection_body_derived() { }


		template<class... Args1>
		void initialize(Args1&&... args)
		{
			new((void*)&fFuncStore) FuncStorage(std::forward<Args1>(args)...);
		}

		// bind version
		template<class Func1, class... Args1>
		void initialize_bind(Func1&& func, Args1&&... args)
		{

			new((void*)&fFuncStore) FuncStorage(
				std::bind(std::forward<Func1>(func), std::forward<Args1>(args)...));
		}

		~connection_body_derived()
		{
		}


		Return Invoke(Args&&... args) override final
		{
			// inline only if the fFunc(...) is a tiny function
			return fFuncStore(std::forward<Args>(args)...);
		}

		void Destroy() override final
		{
			fFuncStore.~FuncStorage();
		}

	};

	namespace details {
		template<class Return, class... Args>
		struct auto_lock {
			connection_body<Return, Args...> &fB;
			auto_lock(connection_body<Return, Args...> &b) : fB(b) {
				b.IncStrongRef(); // keep functor alive
			}
			~auto_lock() {
				fB.DecStrongRef();
			}
		};
	}

	class connection {
	public:
		using connection_type = connection;

		// allow empty connection, so user can initialize connection later
		// just we initialize fBody later
		connection() {
			fBody = nullptr;
		}

		connection(connection const &r) {
			if (r.fBody)
				r.fBody->IncWeakRef();
			fBody = r.fBody;
		}

		connection(connection &&r) {
			fBody = r.fBody;
			if (r.fBody)
				r.fBody = nullptr;
		}

		connection &operator=(connection &&r)
		{
			// if this == &r
			// there are at least 2 weak refs
			if (fBody)
				fBody->DecWeakRef();
			fBody = r.fBody;
			if (r.fBody)
				r.fBody = nullptr;
			return *this;
		}

		connection &operator=(connection const &r) {
			if (r.fBody)
				r.fBody->IncWeakRef();
			if (fBody)
				fBody->DecWeakRef();
			fBody = r.fBody;
			return *this;
		}

		connection(linked_connection_body_base *body) : fBody(body) {
			fBody->IncWeakRef();
		}

		~connection() {
			if (fBody) {
				fBody->DecWeakRef();
				fBody = nullptr;
			}
		}

		bool connected() {
			return fBody && fBody->fConnected;
		}

		void disconnect() {
			if (fBody) {
				// try obtain the body
				if (fBody->fStrongRef) {
					fBody->Disconnect();
					// body would not be deleted, becuase we have a weak ref
					// but the functor may be destroied immediately
				}
				fBody->DecWeakRef();
				fBody = nullptr;
			}
		}

		linked_connection_body_base *fBody;
	};

	template<class Signature, class Return, class... Args>
	struct signal_impl {
	public:
		using connection_type = connection;
		using connection_body_type = connection_body<Return, Args...>;
		using connection_bodies_type = details::linked;

		connection_bodies_type fConnectionBodies;

		signal_impl() { };
		signal_impl(signal_impl const &) = delete;
		signal_impl &operator=(signal_impl const &) = delete;

		signal_impl(signal_impl &&r) {
			if (!r.fConnectionBodies.empty()) {
				fConnectionBodies.fNext = r.fConnectionBodies.fNext;
				fConnectionBodies.fPrev = r.fConnectionBodies.fPrev;
				fConnectionBodies.fNext->fPrev = &fConnectionBodies;
				fConnectionBodies.fPrev->fNext = &fConnectionBodies;
				r.fConnectionBodies.fNext = &r.fConnectionBodies;
				r.fConnectionBodies.fPrev = &r.fConnectionBodies;
			}
		}
		signal_impl &operator=(signal_impl &&r) {
			if (!r.fConnectionBodies.empty()) {
				fConnectionBodies.fNext = r.fConnectionBodies.fNext;
				fConnectionBodies.fPrev = r.fConnectionBodies.fPrev;
				fConnectionBodies.fNext->fPrev = &fConnectionBodies;
				fConnectionBodies.fPrev->fNext = &fConnectionBodies;
				r.fConnectionBodies.fNext = &r.fConnectionBodies;
				r.fConnectionBodies.fPrev = &r.fConnectionBodies;
			}
		}

		~signal_impl() {
			disconnect_all();
		}

		//static_assert(std::is_move_assignable_v<signal>, "");
		//static_assert(std::is_move_constructible_v<signal>, "");

		template<class Func>
		std::enable_if_t<
			std::is_convertible<
			    decltype(std::declval<Func>()(std::declval<Args>()...)),
			    Return
			>::value,
			connection_type> connect(Func&& func)
		{
			using Binder = std::decay_t<Func>;
			connection_body_derived<Binder, Return, Args...> *ptr = new connection_body_derived<Binder, Return, Args...>();
			ptr->initialize(std::forward<Func>(func));
			fConnectionBodies.push_back(ptr);
			return ptr;
		}

		template<class Obj, class... Args1>
		std::enable_if_t<
			std::is_convertible<
			    decltype(std::declval<Obj>()(std::declval<Args>()...)),
			    Return
			>::value,
			connection_type> connect_emplace(Args1&&... args)
		{
			using Binder = Obj;
			connection_body_derived<Binder, Return, Args...> *ptr = new connection_body_derived<Binder, Return, Args...>();
			ptr->initialize(std::forward<Args1>(args)...);
			fConnectionBodies.push_back(ptr);
			return ptr;
		}

		// VS won't inline here
		// it's not good, becuase there is only invocation point
		template<class Func1, class... Args1>
		auto connect_bind(Func1&& func, Args1&&... args)
			-> std::enable_if_t<
			std::is_convertible<
			decltype(std::bind(std::forward<Func1>(func), std::forward<Args1>(args)...)(std::declval<Args>()...)),
			Return
			>::value,
			connection_type>
		{
			// all things expaned! good!
			using Binder = decltype(std::bind(std::forward<Func1>(func), std::forward<Args1>(args)...));
			connection_body_derived<Binder, Return, Args...> *ptr = new connection_body_derived<Binder, Return, Args...>();
			ptr->initialize_bind(std::forward<Func1>(func), std::forward<Args1>(args)...);
			fConnectionBodies.push_back(ptr);
			return ptr;
		}

		void disconnect_all_slots() { disconnect_all(); }
		
		void disconnect_all()
		{
			auto *end = &fConnectionBodies;
			for (auto p = fConnectionBodies.fNext; p != end; )
			{
				// down cast
				connection_body_type &body = static_cast<connection_body_type &>(*p);
				p = p->fNext;
				body.Disconnect();
			}
		}

		// VS won't inline here
		// it's good, because there are many invocation points!
		void operator()(Args... args) const
		{
			auto *end = &fConnectionBodies;
			for (auto p = fConnectionBodies.fNext; p != end; )
			{
				// down cast
				connection_body_type &body = static_cast<connection_body_type &>(*p);

				if (body.fConnected) {
					details::auto_lock<Return, Args...> auto_lock(body);  //prevent unlink from list
					// impossible inline
					body.Invoke(std::forward<Args>(args)...);
					p = p->fNext;
				}

			}
		}

		std::enable_if_t< !std::is_same<Return, void>::value,
			void> operator()(Args... args,
				std::conditional_t<std::is_same<Return, void>::value, int, Return> &last) const
		{
			auto const *end = &fConnectionBodies;
			auto p = fConnectionBodies.fNext;

			for (; p != end && !static_cast<connection_body_type*>(p)->fConnected; p = p->fNext) { }
			if (p == end) return;

			for (;;)
			{
				connection_body_type &body = static_cast<connection_body_type &>(*p);

				details::auto_lock<Return, Args...> auto_lock(body); //prevent unlink from list
				auto tmp = body.Invoke(std::forward<Args>(args)...);
				p = p->fNext;     // get next

				for (; p != end && !static_cast<connection_body_type*>(p)->fConnected; p = p->fNext) {}
				if (p == end) {
					last = std::move(tmp);
					break;
				}

			}
		}

	};

	template<class Signature>
	struct get_signal_impl;

	template<class Return, class... Args>
	struct get_signal_impl<Return(Args...)> {
		using type = signal_impl<Return(Args...), Return, Args...>;
	};

	template<class Signature>
	class signal : public get_signal_impl<Signature>::type
	{
	public:
		using base_type = typename get_signal_impl<Signature>::type;
		signal() : base_type() { }
		signal(signal&& r) : base_type(std::move((base_type&&)r)) { }
		signal& operator=(signal&& r) { (base_type&)(*this) = std::move(r); }
	};

}

#endif // TISS_H
