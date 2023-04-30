

#ifndef LUABIND_ITERATOR_PAIR_POLICY_HPP_INCLUDED
#define LUABIND_ITERATOR_PAIR_POLICY_HPP_INCLUDED

#include <luabind/config.hpp>
#include <luabind/detail/policy.hpp>
#include <luabind/detail/implicit_cast.hpp>
#include <luabind/detail/convert_to_lua.hpp>

namespace luabind { namespace detail 
{
	template<class Iter>
	struct iterator_pair_state
	{
		typedef iterator_pair_state<Iter> self_t;

		static int step(lua_State* L)
		{
			self_t& state = *static_cast<self_t*>(lua_touserdata(L, lua_upvalueindex(1)));

			if (state.start == state.end)
			{
				lua_pushnil(L);
				return 1;
			}
			else
			{
				convert_to_lua(L, (*state.start).first);
				convert_to_lua(L, (*state.start).second);
				++state.start;
				return 2;
			}
		}

		iterator_pair_state(const Iter& s, const Iter& e)
			: start(s)
			, end(e)
		{}

		Iter start;
		Iter end;
	};

	struct iterator_pair_converter
	{
		template<class T>
		void apply(lua_State* L, const T& c)
		{
			typedef typename T::const_iterator iter_t;
			typedef iterator_pair_state<iter_t> state_t;

			// note that this should be destructed, for now.. just hope that iterator
			// is a pod
			void* iter = lua_newuserdata(L, sizeof(state_t));
			new (iter) state_t(c.begin(), c.end());
			lua_pushcclosure(L, state_t::step, 1);
		}

		template<class T>
		void apply(lua_State* L, T& c)
		{
			typedef typename T::iterator iter_t;
			typedef iterator_pair_state<iter_t> state_t;

			// note that this should be destructed, for now.. just hope that iterator
			// is a pod
			void* iter = lua_newuserdata(L, sizeof(state_t));
			new (iter) state_t(c.begin(), c.end());
			lua_pushcclosure(L, state_t::step, 1);
		}
	};

	struct iterator_pair_policy : conversion_policy<0>
	{
		static void precall(lua_State*, const index_map&) {}
		static void postcall(lua_State*, const index_map&) {}

		template<class T, class Direction>
		struct generate_converter
		{
			typedef iterator_pair_converter type;
		};
	};

}}

namespace luabind
{
	namespace
	{
		LUABIND_ANONYMOUS_FIX detail::policy_cons<detail::iterator_pair_policy, detail::null_type> return_stl_pair_iterator;
	}
}

#endif // LUABIND_ITERATOR_PAIR_POLICY_HPP_INCLUDED

