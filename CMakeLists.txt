cmake_minimum_required(VERSION 3.10)

project(zero)

add_compile_options(-g)

add_executable(zero 
    main.c
    zero.h
    vec.h
    vec.c
    map.h
    map.c
)