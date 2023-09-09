/**
 * 
 *                                  * Usage * 
 * 
 * template <Role role>
 * ClientOnlyFunctionType<role> clientOnlyFunction()
 * {
 *  std::cout << "This function is only available to clients" << std::endl;
 * }
*/

#pragma once
#include <transport_enums.hpp>

template <Role role>
using ClientOnlyFunctionType = std::enable_if_t<role == Role::Client, void>;

template <Role role>
using ServerOnlyFunctionType = std::enable_if_t<role == Role::Server, void>;
