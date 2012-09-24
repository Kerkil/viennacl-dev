#ifndef VIENNACL_GENERATOR_CREATE_KERNEL_HPP
#define VIENNACL_GENERATOR_CREATE_KERNEL_HPP

/* =========================================================================
   Copyright (c) 2010-2012, Institute for Microelectronics,
                            Institute for Analysis and Scientific Computing,
                            TU Wien.

                            -----------------
                  ViennaCL - The Vienna Computing Library
                            -----------------

   Project Head:    Karl Rupp                   rupp@iue.tuwien.ac.at

   (A list of authors and contributors can be found in the PDF manual)

   License:         MIT (X11), see file LICENSE in the base directory
============================================================================= */

/** @file viennacl/generator/get_kernels_infos.hpp
 *  @brief Provides information about kernels
 *
 *  Generator code contributed by Philippe Tillet
 */

// #include "kernel_utils.hpp"

#include <map>

#include "viennacl/generator/operators.hpp"
#include "viennacl/generator/symbolic_types.hpp"
#include "viennacl/generator/tree_operations.hpp"
#include "viennacl/generator/tokens_management.hpp"
#include "viennacl/generator/make_code.hpp"
#include "viennacl/generator/meta_tools/typelist.hpp"
#include "viennacl/generator/result_of.hpp"

namespace viennacl 
{
namespace generator
{

typedef std::multimap<std::string, std::pair<unsigned int, result_of::runtime_wrapper*> > runtime_wrappers_t;

template<class T>
struct get_head{
    typedef T Result;
};

template<class Head, class Tail>
struct get_head<typelist<Head, Tail> >
{
    typedef Head Result;
};



template<class T>
struct transform_inner_prod
{
    typedef T Result;
};

template<class LHS, class RHS>
struct transform_inner_prod<compound_node<LHS,inner_prod_type,RHS> >
{
    typedef inner_prod_impl_t<compound_node<LHS,inner_prod_type,RHS> > Result;
};

template<class TreeList, class Res, int CurrentIndex=0>
struct register_kernels;

template<class Head,class Tail, class Res,int CurrentIndex>
struct register_kernels<typelist<Head, Tail>,Res,CurrentIndex >
{
private:
    typedef typelist<Head, Tail> self_type;
public:
    template<class T, class List, int Index>
    struct add_to_res
    {
        //Gets the typelist at Index
        typedef typename typelist_utils::type_at<List,Index>::Result Tmp;

        //Fuses it with the argument provided
        typedef typename typelist_utils::fuse<Tmp,T>::Result TmpRes;

        //Replace the former typelist with the new typelist
        typedef typename typelist_utils::replace<List,Tmp,TmpRes>::Result ResultIfTmpNotNull;
        typedef typename typelist_utils::append<List,T>::Result ResultIfTmpNull;
        typedef typename get_type_if<ResultIfTmpNull,ResultIfTmpNotNull,is_null_type<Tmp>::value>::Result Result;
    };

private:
    typedef typename tree_utils::extract_if<Head,is_inner_product_leaf>::Result InProds;
    typedef typename add_to_res<typename typelist_utils::ForEachType<InProds,transform_inner_prod>::Result,Res,CurrentIndex - 1>::Result TmpNewRes;
    static const bool inc = tree_utils::count_if<typename get_head<Tail>::Result, is_inner_product_leaf>::value
                            + tree_utils::count_if<Head,is_product_leaf>::value;
public:
    typedef typename add_to_res<typelist<Head,NullType>,TmpNewRes,CurrentIndex>::Result NewRes;
    typedef typename register_kernels<Tail,NewRes,CurrentIndex+inc>::Result Result;
};

template<class Res, int CurrentIndex>
struct register_kernels<NullType,Res,CurrentIndex>
{
    typedef Res NewRes;
    typedef Res Result;
};

template<class ARG>
struct program_infos
{

    static const bool first_has_ip = tree_utils::count_if<typename ARG::Head,is_inner_product_leaf>::value;
    typedef typename register_kernels<ARG,NullType,first_has_ip>::Result          KernelsList;

    template<class Operations>
    struct fill_args
    {
        typedef typename tree_utils::extract_if<Operations,is_kernel_argument>::Result IntermediateType;
        typedef typename typelist_utils::no_duplicates<IntermediateType>::Result Arguments;

        template<class U>
        struct functor{
        private:
            typedef typename result_of::expression_type<U>::Result ExpressionType;
        public:
            static void execute(unsigned int & arg_pos, runtime_wrappers_t & runtime_wrappers, std::string const & name)
            {
                runtime_wrappers.insert(runtime_wrappers_t::value_type(name,
                                                                       std::make_pair(arg_pos,
                                                                                      ExpressionType::runtime_descriptor())
                                                                       )
                                        );
                arg_pos += ExpressionType::n_args();
            }
        };

        static void execute(runtime_wrappers_t & runtime_wrappers,std::string const & operation_name)
        {
            unsigned int arg_pos = 0;
            std::string current_kernel_name("__" + operation_name + "_k" + to_string(typelist_utils::index_of<KernelsList,Operations>::value));
            typelist_utils::ForEach<Arguments,functor>::execute(arg_pos,runtime_wrappers,current_kernel_name);
            if(tree_utils::count_if<Operations,is_inner_product_leaf>::value){
                runtime_wrappers.insert(runtime_wrappers_t::value_type(current_kernel_name,
                                                                       std::make_pair(arg_pos,
                                                                                      new result_of::shared_memory_wrapper())));
            }
        }


    };

    template<class Operations>
    struct fill_sources
    {
    private:
            typedef typename tree_utils::extract_if<Operations,is_kernel_argument>::Result IntermediateType;
            typedef typename typelist_utils::no_duplicates<IntermediateType>::Result Arguments;

    public:
        template<class TList>
        struct header_code
        {

            template<class T>
            struct functor{
                static void execute(std::string & res,bool & is_first){
                    if(is_first){
                        res+=T::kernel_arguments();
                        is_first=false;
                    }
                    else{
                        res+=", "+T::kernel_arguments();
                    }
                }
            };

        public:
            static const std::string value ( std::string const & name )
            {
                std::string res;
                res+="__kernel void " + name + "(\n";
                bool state=true;
                typelist_utils::ForEach<Arguments,functor>::execute(res,state);
                if(tree_utils::count_if<TList,is_inner_product_leaf>::value)
                    res+=",__local float* shared_memory_ptr\n";
                res+=")\n";
                return res;
            }
        };


        static void execute(std::map<std::string,std::string> & sources,std::string const & operation_name)
        {
            std::string current_kernel_name("__" + operation_name + "_k" + to_string(typelist_utils::index_of<KernelsList,Operations>::value));
            sources.insert(std::make_pair(current_kernel_name,
                                          header_code<Operations>::value(current_kernel_name)
                                          +body_code<Operations>::value()));
        }
    };


    static void fill(std::string const & operation_name, std::map<std::string,std::string> & sources, runtime_wrappers_t & runtime_wrappers)
    {
        std::cout << KernelsList::name() << std::endl;
        typelist_utils::ForEach<KernelsList,fill_sources>::execute(sources,operation_name);
        typelist_utils::ForEach<KernelsList,fill_args>::execute(runtime_wrappers,operation_name);
    }
};



} // namespace generator
} // namespace viennacl
#endif
