//********** Copyright © 2023 Sean Caroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : An example executable.
//*
//*
//****************************************************************************

#include <iostream>                                      // For cout
#include <sc/example.hpp>                                // For example

//****************************************************************************

int main(int, const char*[])
{
  std::cout << "example(3) = " << sc::example(3) << std::endl;
}

//****************************************************************************
