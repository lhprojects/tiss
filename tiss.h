#ifndef TISS_H
#define TISS_H

#include <utility>
#include <memory>
#include <type_traits>
#include <functional>
#include <tuple>

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

	template<class Result, class... Args>
	struct signal_impl;


	template<class Signature>
	struct _Get_Tuple;

	template<class Return, class... Args>
	struct _Get_Tuple< Return(Args...) >
	{
		using type = std::tuple< Args... >;
	};

	template<class Result, class... Args>
	class _Result_iterator_impl
	{
	public:
		typedef Result _Signature(Args...);
		using _Signal = signal_impl<Result, Args...>;
		using _Node = details::linked;
		using _Body = connection_body<Result, Args...>;
		using _Tuple = std::tuple<Args...>;

		_Node *_fNode;
		_Node const *_fHead;
		_Tuple *_fArgs;
		_Result_iterator_impl(
			_Node *node,
			_Node const *head,
			_Tuple *args) 
			: _fNode(node), _fHead(head), _fArgs(args)
		{
		}

		//static_assert(std::is_copy_constructible_v<_Result_iterator_impl>);
		//static_assert(std::is_copy_assignable_v<_Result_iterator_impl>);

		bool operator!=(_Result_iterator_impl const &r) const
		{
			return _fNode != r._fNode;
		}

		_Result_iterator_impl &operator++()
		{
			_fNode = _fNode->fNext;
			for (; _fNode != _fHead && !static_cast<_Body *>(_fNode)->fConnected; _fNode = _fNode->fNext) { }
			return *this;
		}

		_Result_iterator_impl operator++(int)
		{
			_Result_iterator_impl t = *this
			++(*this);
			return t;
		}

		template<std::size_t... I>
		Result _Invoke(_Body &body, _Tuple *args, std::index_sequence<I>...) const
		{
			// make a copy and invoke
			return body.Invoke(std::forward<Args>(std::get<I>(_Tuple(*args))...)...);
		}

		Result operator*() const
		{
			auto &body = static_cast<_Body &>(*_fNode);
			return _Invoke(body, _fArgs, std::make_index_sequence<sizeof...(Args)>());
		}
	};

	template<class Signature>
	struct _Get_result_iterator_impl;

	template<class Return, class... Args>
	struct _Get_result_iterator_impl<Return(Args...)> {
		using type = _Result_iterator_impl<Return, Args...>;
	};

	template<class Signature>
	struct result_iterator : _Get_result_iterator_impl<Signature>::type
	{
		using _MyBase = typename _Get_result_iterator_impl<Signature>::type;
		using _Signal = typename _MyBase::_Signal;
		using _Node = details::linked;
		using _Body = typename _MyBase::_Body;
		using _Tuple = typename _MyBase::_Tuple;

		result_iterator(
			_Node *node,
			_Node const *head,
			_Tuple *args) : _MyBase(node, head, args)
		{
		}
	};

	template<class Signature>
	struct result_range
	{
		using _Tuple = typename _Get_Tuple<Signature>::type;
		using _Iter = result_iterator<Signature>;
		using _Node = details::linked;

		_Tuple _fTuple;
		_Iter _fBegin;
		_Iter _fEnd;

		template<class... Args1>
		result_range(_Node *b, _Node const *e, Args1&&... args) :
			_fTuple(std::forward<Args1>(args)...),
			_fBegin(b, e, &_fTuple), 
			_fEnd((_Node*)e, e, &_fTuple)
		{
		}

		_Iter begin() const
		{
			return _fBegin;
		}

		_Iter end() const
		{
			return _fEnd;
		}
	};

	template<class Return, class... Args>
	struct signal_impl {
	public:
		typedef Return Signature (Args...);
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

		size_t num_connections()
		{
			size_t num = 0;
			auto *end = &fConnectionBodies;
			for (auto p = fConnectionBodies.fNext; p != end; )
			{
				// down cast
				connection_body_type &body = static_cast<connection_body_type &>(*p);
				p = p->fNext;
				if (body.fConnected) num += 1;
			}
			return num;
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
		

		template<class = std::enable_if_t< !std::is_same<Return, void>::value, void>>
		bool invoke_and_get_last_result(Args... args,
				std::conditional_t<std::is_same<Return, void>::value, int, Return> &last) const
		{
			auto const *end = &fConnectionBodies;
			auto p = fConnectionBodies.fNext;

			for (; p != end && !static_cast<connection_body_type*>(p)->fConnected; p = p->fNext) { }
			if (p == end) return false;

			for (;;)
			{
				connection_body_type &body = static_cast<connection_body_type &>(*p);

				details::auto_lock<Return, Args...> auto_lock(body); //prevent unlink from list
				auto tmp = body.Invoke(std::forward<Args>(args)...);
				p = p->fNext;     // get next

				for (; p != end && !static_cast<connection_body_type*>(p)->fConnected; p = p->fNext) {}
				if (p == end) {
					last = std::move(tmp);
					return false;
				}

			}
		}


		template<class ResultHanler, class = decltype(std::declval<ResultHanler&>()(std::declval<Return>())) >
		void operator()(Args... args,
				ResultHanler &handler) const
		{
			auto *end = &fConnectionBodies;
			for (auto p = fConnectionBodies.fNext; p != end; )
			{
				// down cast
				connection_body_type &body = static_cast<connection_body_type &>(*p);

				if (body.fConnected) {
					details::auto_lock<Return, Args...> auto_lock(body);  //prevent unlink from list
																		  // impossible inline
					handler(body.Invoke(std::forward<Args>(args)...));
					p = p->fNext;
				}

			}
		}

		result_range<Signature> invoke_and_get_range(Args... args) const
		{
			auto p = fConnectionBodies.fNext;
			auto end = &fConnectionBodies;
			for (; p != end && !static_cast<connection_body_type*>(p)->fConnected; p = p->fNext) {}

			// move if possible
			return result_range<Signature>(p, end, std::forward<Args>(args)...);
		}

	};

	template<class Signature>
	struct get_signal_impl;

	template<class Return, class... Args>
	struct get_signal_impl<Return(Args...)> {
		using type = signal_impl<Return, Args...>;
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
