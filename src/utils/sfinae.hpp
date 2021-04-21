#pragma once

namespace osc {
    /**
     * what you are about to see (using SFINAE to test whether a class has a texcoord member)
     * is better described with a diagram:

            _            _.,----,
    __  _.-._ / '-.        -  ,._  \)
    |  `-)_   '-.   \       / < _ )/" }
    /__    '-.   \   '-, ___(c-(6)=(6)
    , `'.    `._ '.  _,'   >\    "  )
    :;;,,'-._   '---' (  ( "/`. -='/
    ;:;;:;;,  '..__    ,`-.`)'- '--'
    ;';:;;;;;'-._ /'._|   Y/   _/' \
      '''"._ F    |  _/ _.'._   `\
             L    \   \/     '._  \
      .-,-,_ |     `.  `'---,  \_ _|
      //    'L    /  \,   ("--',=`)7
     | `._       : _,  \  /'`-._L,_'-._
     '--' '-.\__/ _L   .`'         './/
                 [ (  /
                  ) `{
       snd        \__)

     */
    template<typename>
    struct sfinae_true : std::true_type {};
    namespace detail {
        template<typename T>
        static auto test_has_texcoord(int) -> sfinae_true<decltype(std::declval<T>().texcoord)>;
        template<typename T>
        static auto test_has_texcoord(long) -> std::false_type;
    }
    template<typename T>
    struct has_texcoord : decltype(detail::test_has_texcoord<T>(0)) {};
}
