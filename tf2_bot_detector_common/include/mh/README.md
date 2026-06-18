# stuff

https://github.com/surepy/tf2bd_mh_stuff
https://github.com/PazerOP/stuff

Sometimes I find myself rewriting different versions of the same logic in lots of unrelated projects I work on. Here's some of that stuff.

This "library" can operate in either header-only or static/shared library mode. In header-only mode, it should be sufficient to just copy individual `.h` and `.inl` files that you want to use, without using the CMake script.

Efforts have been made to reduce interdependencies between headers in the codebase, although some fairly strong interdependencies do exist for the sake of reducing code duplication (coroutine code, especially).

This library experiences a lot of API churn. No API stability is guaranteed, and none should be expected. I regularly make significant structural changes to support applications I develop with this code. To save yourself from having a bad time, only look at this library as something to base your own code off of, or expect to make modifications yourself.
