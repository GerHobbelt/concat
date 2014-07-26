/*
 *  CONCAT 
 *  Version: 2014-23-07
 *  ----------------------------------------------------------
 *  Copyright (c) 2014 José Manuel Barroso Galindo. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef THEYPSILON_CONCAT
#define THEYPSILON_CONCAT

#include <sstream>
#include <tuple>

namespace theypsilon {

    template <typename CharT>
    struct separator_t {
        const CharT* sep;
        constexpr explicit separator_t(const CharT* s) noexcept: sep{s} {} 
    };

    template <typename CharT>
    constexpr separator_t<CharT> separator(const CharT* s) {
        return separator_t<CharT>(s);
    }

    namespace sep {
        constexpr char none [] = "";
        constexpr char space[] = " ";
        constexpr char endl [] = "\n";
        constexpr char coma [] = ", ";
        constexpr char plus [] = " + ";
    };

    namespace { // is_container, is_stringstream
        template<typename T>
        struct has_const_iterator {
        private:
            typedef char                      yes;
            typedef struct { char array[2]; } no;

            template<typename C> static yes test(typename C::const_iterator*);
            template<typename C> static no  test(...);
        public:
            static const bool value = sizeof(test<T>(0)) == sizeof(yes);
            typedef T type;
        };

        template <typename T>
        struct has_begin_end {
            template<typename C> static char (&f(typename std::enable_if<
            std::is_same<decltype(static_cast<typename C::const_iterator (C::*)() const>(&C::begin)),
            typename C::const_iterator(C::*)() const>::value, void>::type*))[1];

            template<typename C> static char (&f(...))[2];

            template<typename C> static char (&g(typename std::enable_if<
            std::is_same<decltype(static_cast<typename C::const_iterator (C::*)() const>(&C::end)),
            typename C::const_iterator(C::*)() const>::value, void>::type*))[1];

            template<typename C> static char (&g(...))[2];

            static bool const beg_value = sizeof(f<T>(0)) == 1;
            static bool const end_value = sizeof(g<T>(0)) == 1;
        };

        template<typename T, typename CharT>
        constexpr bool is_writable_stream() {
            return  std::is_same<T, std::basic_ostringstream<CharT>>::value || 
                    std::is_same<T, std::basic_stringstream <CharT>>::value ||
                    std::is_same<T, std::basic_ostream<CharT>>::value;
        }

        template<typename T, typename CharT = char> 
        constexpr bool is_stringstream() {
            return  std::is_same<T, std::basic_istringstream<CharT>>::value || 
                    std::is_same<T, std::basic_ostringstream<CharT>>::value ||
                    std::is_same<T, std::basic_stringstream <CharT>>::value;
        }

        template<typename T> 
        constexpr bool is_array() {
            return std::is_array<T>::value;
        }

        template<typename T> 
        constexpr bool is_char_type() {
            return  std::is_same<typename std::decay<T>::type, char    >::value || 
                    std::is_same<typename std::decay<T>::type, wchar_t >::value || 
                    std::is_same<typename std::decay<T>::type, char16_t>::value || 
                    std::is_same<typename std::decay<T>::type ,char32_t>::value;
        }

        template<typename T, typename CharT = char>
        constexpr bool is_c_str() { 
            return  std::is_same<typename std::decay<T>::type, CharT const *>::value ||
                    std::is_same<typename std::decay<T>::type, CharT       *>::value;
        }

        template<typename T> 
        constexpr bool is_char_sequence() {
            return  is_c_str<T,     char>() ||
                    is_c_str<T,  wchar_t>() ||
                    is_c_str<T, char16_t>() ||
                    is_c_str<T, char32_t>();
        }

        template<typename T> 
        constexpr bool is_container() {
            return (has_const_iterator<T>::value && 
                    has_begin_end<T>::beg_value  && 
                    has_begin_end<T>::end_value  &&
                    !std::is_same<T, std::string>::value &&
                    !is_stringstream<T>()) 
            || (std::is_array<T>::value && !is_char_sequence<T*>());
        }

        template<typename T> 
        constexpr bool can_be_false() {
            return (is_stringstream<T>() || is_char_sequence<T>()) && !std::is_array<T>::value;
        }

        template <typename T>
        constexpr bool is_string() {
            return  std::is_same<T,std::string   >::value ||
                    std::is_same<T,std::wstring  >::value ||
                    std::is_same<T,std::u16string>::value ||
                    std::is_same<T,std::u32string>::value;
        }

        template <typename T>
        constexpr bool is_basic_type() {
            return std::is_scalar<T>::value || is_string<T>();
        }

        template <typename T, template <typename...> class Template>
        struct is_specialization_of : std::false_type {};

        template <template <typename...> class Template, typename... Args>
        struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

        template <typename T>
        constexpr bool is_modifier() {
            return  !is_container    <T>() && !is_stringstream    <T>() && 
                    !is_char_sequence<T>() && !is_basic_type<T>() &&
                    !std::is_array<T>::value && 
                    !is_specialization_of<T, std::tuple>::value;
        }
    }

    namespace { // concat_intern

        template <typename CharT, char head, char... tail>
        std::basic_string<CharT> get_separator() { return {head, tail...}; }

        template <typename W, typename S>
        void separate(W& writter, const S* separator) { 
            if (separator) writter << separator;
        }

        template <typename W, typename S>
        void separate(W& writter, const S& separator) { 
            writter << separator;
        }

        template <typename CharT, typename W>
        std::basic_string<CharT> concat_to_string(const W& writter) {
            return writter.good() ? writter.str() : std::basic_string<CharT>();
        }

        namespace {
            template <typename CharT, typename W, typename S, typename T>
            void concat_intern_write(W& writter, const S& separator, bool b, const T& v);

            template <typename CharT, typename W, typename S, typename T,
                typename std::enable_if<is_char_sequence<T*>(), T>::type* = nullptr>
            void concat_intern_recursion(W& writter, const S& separator, bool, const T* v) {
                if (v) writter << v;
            }

            template <typename CharT, typename W, typename S, typename T,
                typename std::enable_if<(!is_container<T>() && !is_stringstream<T>() && !is_char_sequence<T>()) || is_modifier<T>(), T>::type* = nullptr>
            void concat_intern_recursion(W& writter, const S& separator, bool, const T& v) {
                writter << v;
            }

            template <typename CharT, typename W, typename S, typename T,
                typename std::enable_if<is_stringstream<T>(), T>::type* = nullptr>
            void concat_intern_recursion(W& writter, const S& separator, bool, const T& v) {
                if (v.good()) writter << concat_to_string<CharT>(v);
                else writter.setstate(v.rdstate());
            }

            template <typename CharT, typename W, typename S, typename T,
                typename std::enable_if<is_container<T>(), T>::type* = nullptr>
            void concat_intern_recursion(W& writter, const S& separator, bool, const T& container) {
                auto it = std::begin(container), et = std::end(container);
                while(it != et) {
                    auto element = *it;
                    it++;
                    concat_intern_write<CharT>(writter, separator, it != et, element);
                }
            }

            template <typename CharT, typename W, typename S, typename... Args>
            void concat_intern_recursion(W& writter, const S& separator, bool, const std::tuple<Args...>& v);

            template<unsigned N, unsigned Last>
            struct tuple_printer {
                template<typename CharT, typename W, typename S, typename T>
                static void print(W& writter, const S& separator, bool b, const T& v) {
                    concat_intern_write<CharT>(writter, separator, true, std::get<N>(v));
                    tuple_printer<N + 1, Last>::template print<CharT>(writter, separator, b, v);
                }
            };

            template<unsigned N>
            struct tuple_printer<N, N> {
                template<typename CharT, typename W, typename S, typename T>
                static void print(W& writter, const S& separator, bool, const T& v) {
                    concat_intern_write<CharT>(writter, separator, false, std::get<N>(v));
                }
            };

            template <typename CharT, typename W, typename S, typename... Args>
            inline void concat_intern_recursion(W& writter, const S& separator, bool b, const std::tuple<Args...>& v) {
                tuple_printer<0, sizeof...(Args) - 1>::template print<CharT>(writter, separator, b, v);
            }

            template <typename CharT, typename W, typename S, typename T, typename... Args>
            void concat_intern_recursion(W& writter, const S& separator, bool, const T& head, const Args&... tail) {
                concat_intern_write<CharT>(writter, separator, true, head);
                concat_intern_recursion<CharT>(writter, separator, false, tail...);
            }

            template <typename CharT, typename W, typename S, typename T>
            inline void concat_intern_write(W& writter, const S& separator, bool b, const T& v) {
                concat_intern_recursion<CharT>(writter, separator, b, v);
                if (b && !is_modifier<T>()) separate(writter, separator);
            }

            template <typename CharT, typename S, typename T, typename... Args,
                typename std::enable_if<is_writable_stream<T, CharT>() == true, T>::type* = nullptr>
            std::basic_string<CharT> concat_intern(const S& separator, T& writter, const Args&... seq) {
                concat_intern_recursion<CharT>(writter, separator, false, seq...);
                return concat_to_string<CharT>(writter);
            }
        }

        template <typename CharT, typename S, typename... Args>
        std::basic_string<CharT> concat_intern(const S& separator, const Args&... seq) {
            std::basic_ostringstream<CharT> writter;
            return concat_intern<CharT>(separator, writter, seq...);
        }
    }

    template <typename CharT = char, typename... Args>
    std::basic_string<CharT> concat(const separator_t<CharT>& sep, Args&&... seq) {
        return concat_intern<CharT>(sep.sep, std::forward<Args>(seq)...);
    }

    template <char head, char... tail, typename F, typename... Args,
        typename = typename std::enable_if<std::is_same<F, separator_t<char>>::value == false, F>::type>
    std::basic_string<char> concat(F&& first, Args&&... rest) {
        return concat_intern<char>(get_separator<char, head, tail...>(), std::forward<F>(first), std::forward<Args>(rest)...);
    }

    template <const char* sep, typename F, typename... Args,
        typename = typename std::enable_if<std::is_same<F, separator_t<char>>::value == false, F>::type>
    std::basic_string<char> concat(F&& first, Args&&... rest) {
        return concat_intern<char>(sep, std::forward<F>(first), std::forward<Args>(rest)...);
    }

    template <typename CharT = char, typename F, typename... Args,
        typename = typename std::enable_if<std::is_same<F, separator_t<CharT>>::value == false, F>::type>
    std::basic_string<CharT> concat(F&& first, Args&&... rest) {
        return concat_intern<CharT>((const char*)nullptr, std::forward<F>(first), std::forward<Args>(rest)...);
    }

    template <std::ostream& sep (std::ostream&), typename F, typename... Args,
        typename = typename std::enable_if<std::is_same<F, separator_t<char>>::value == false, F>::type>
    std::basic_string<char> concat(F&& first, Args&&... rest) {
        return concat_intern<char>(sep, std::forward<F>(first), std::forward<Args>(rest)...);
    }

}

#endif