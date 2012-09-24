#ifndef VIENNACL_GENERATOR_MAKE_CODE_HPP
#define VIENNACL_GENERATOR_MAKE_CODE_HPP

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


/** @file viennacl/generator/make_code.hpp
 *  @brief Definition of code generation policies.
 *
 *  Generator code contributed by Philippe Tillet
 */

#include "viennacl/generator/forwards.h"
#include "viennacl/generator/meta_tools/utils.hpp"
#include "viennacl/generator/symbolic_types.hpp"
#include "viennacl/generator/result_of.hpp"
#include "viennacl/generator/tree_operations.hpp"

namespace viennacl
{
namespace generator
{

template <class T>
struct inner_prod_impl_t
{
    typedef T Type;

    static const std::string name()
    {
        return T::name();
    }

    static const std::string private_value()
    {
        return "private_"+name();
    }

    static const std::string declarations()
    {
        return print_type<typename T::ScalarType,1>::value() + " " + private_value() + "=0;\n" ;
    }

    static const std::string kernel_arguments(){
        return T::kernel_arguments();
    }

    enum { id = T::id };
};

template <class T>
struct make_expression_code
{
    static const std::string value(std::string const & loop_accessor)
    {
        if(loop_accessor == "gid") return  T::gid_val_name();
        else return T::name() + '[' + loop_accessor + ']';
    }
};

template<class T>
struct make_expression_code<inner_prod_impl_t<T> >
{
    static const std::string value(std::string const & loop_accessor)
    {
        return "";
    }
};

template <unsigned int ID, class SCALARTYPE>
struct make_expression_code<cpu_symbolic_scalar<ID,SCALARTYPE> >
{
    static const std::string value(std::string const & loop_accessor)
    {
        return  cpu_symbolic_scalar<ID,SCALARTYPE>::name();
    }
};

template <unsigned int ID,class SCALARTYPE>
struct make_expression_code<gpu_symbolic_scalar<ID,SCALARTYPE> >
{
    static const std::string value(std::string const & loop_accessor)
    {
        return  gpu_symbolic_scalar<ID,SCALARTYPE>::val_name();
    }
};

template <class LHS, class RHS>
struct make_expression_code<compound_node<LHS,inner_prod_type,RHS > >
{
private:
    typedef compound_node<LHS,inner_prod_type,RHS> T;

public:
    static const std::string value(std::string const & loop_accessor)
    {
        return T::local_value();
    }
};

template< >
struct make_expression_code< NullType >
{
    static const std::string value(std::string const & loop_accessor)
    {
        return "";
    }
};

template<class T, std::string (*U)()>
struct make_expression_code< elementwise_modifier_impl<T, U> >
{
    typedef elementwise_modifier_impl<T, U> EW_M;
    static const std::string value ( std::string const & loop_accessor)
    {
        return EW_M::modify(make_expression_code<T>::value(loop_accessor));
    }
};

template<class LHS, class OP, class RHS >
struct make_expression_code<compound_node<LHS, OP, RHS> >
{
    static const std::string value(std::string const & loop_accessor)
    {
        return " ( " + make_expression_code<LHS>::value(loop_accessor)
                + OP::expression_string()
                + make_expression_code<RHS>::value(loop_accessor) + " ) ";
    }
};

template<class LHS, class RHS>
struct make_expression_code<compound_node<LHS, prod_type, RHS> >
{
    static const std::string value(std::string const & loop_accessor)
    {
        return "dp"+compound_node<LHS, prod_type, RHS>::name();
    }
};

template<class LHS, class RHS, unsigned int Alignment>
struct dot_product_impl
{
    static const std::string value(std::string lhs_loop_id,
                                   std::string rhs_loop_id)
    {
        return "dot(" + make_expression_code<LHS>::value(lhs_loop_id) + "," + make_expression_code<RHS>::value(rhs_loop_id) + ")";
    }
};

template<class LHS, class RHS>
struct dot_product_impl<LHS, RHS, 8>
{
    static const std::string value(std::string lhs_loop_id,
                                   std::string rhs_loop_id)
    {
        return "dot(" + make_expression_code<LHS>::value(lhs_loop_id) + ".s0123" + ","
                + make_expression_code<RHS>::value(rhs_loop_id) + ".s0123 )"
                +  " + dot("	+ make_expression_code<LHS>::value(lhs_loop_id) + ".s4567" + ","
                + make_expression_code<RHS>::value(rhs_loop_id) + ".s4567 );"
                ;
    }
};

template<class LHS, class RHS>
struct dot_product_impl<LHS, RHS, 16>
{
    static const std::string value(std::string lhs_loop_id,std::string rhs_loop_id)
    {
        return "dot(" + make_expression_code<LHS>::value(lhs_loop_id) + ".s0123" + ","
                + make_expression_code<RHS>::value(rhs_loop_id) + ".s0123)"
                +"\n	+ dot("	+ make_expression_code<LHS>::value(lhs_loop_id) + ".s4567" + ","
                + make_expression_code<RHS>::value(rhs_loop_id) + ".s4567) "
                +"\n	+ dot("	+ make_expression_code<LHS>::value(lhs_loop_id) + ".s89ab" + ","
                + make_expression_code<RHS>::value ( rhs_loop_id ) + ".s89ab) "
                +"\n	+ dot("	+ make_expression_code<LHS>::value ( lhs_loop_id ) + ".scdef" + ","
                + make_expression_code<RHS>::value ( rhs_loop_id ) + ".scdef)"
                ;
    }
};

template<class LHS, class RHS>
struct dot_product
{
    static const std::string value(std::string lhs_loop_id,std::string rhs_loop_id)
    {
        return dot_product_impl<LHS,RHS,LHS::Alignment>::value(lhs_loop_id,rhs_loop_id);
    }
};

template <class TOKEN>
struct make_code;

template<>
struct make_code<NullType>
{
    static const std::string value(){
        return "";
    }

    static const std::string sum(){
        return "";
    }

    static const std::string reduction(){
        return "";
    }
};

template <class EXPR>
struct make_code<ArithmeticToken<EXPR> >
{
  static const std::string value()
  {
    std::string res;
    res+="\n//Arithmetic Token\n";
    res+=make_expression_code<EXPR>::value("gid") + ";\n";
    return res;
  }
};

template <class T>
struct make_code<InProdToken<T, 1>  >
{
    template<class U>
    struct generate_code_sum{
    private:
        typedef typename U::Type ARG;
        typedef typename ARG::LHS LHS;
        typedef typename ARG::RHS RHS;
    public :
        static void execute(std::string & generated_code)
        {
            generated_code+= U::private_value() + " += " + dot_product<LHS,RHS>::value("gid","gid") + ";\n";
        }
    };


    template<class U>
    struct generate_code_reduction{
    private:
        typedef typename U::Type ARG;
        typedef typename ARG::LHS LHS;
        typedef typename ARG::RHS RHS;
    public :

        static void execute(std::string & generated_code)
        {
            generated_code+=
                    "shared_memory_ptr[get_local_id(0)] = " + U::private_value() + ";\n"
                    "for (unsigned int stride = get_local_size(0)/2; stride > 0; stride /= 2)\n"
                    "{\n"
                    "  barrier(CLK_LOCAL_MEM_FENCE);\n"
                    "  if (get_local_id(0) < stride)\n"
                    "  shared_memory_ptr[get_local_id(0)] += shared_memory_ptr[get_local_id(0)+stride];\n"
                    "}\n"
                    +ARG::name() + "[get_group_id(0)] = shared_memory_ptr[0];";
        }
    };

    static const std::string sum(){
        std::string res;
        typelist_utils::ForEach<T,generate_code_sum>::execute(res);
        return res;
    }

    static const std::string reduction(){
        std::string res;
        typelist_utils::ForEach<T,generate_code_reduction>::execute(res);
        return res;
    }

};

template <class T>
struct make_code<InProdToken<T, 0> >
{
    template<class U>
    struct generate_code{
    private:
        typedef typename U::LHS LHS;
        typedef typename U::RHS RHS;

    public:

        static void execute(std::string & generated_code)
        {
            generated_code+=
                    "{\n"
                    "   float sum = 0;\n"
                    "   for (unsigned int i = get_local_id(0) ; i<get_num_groups(0) ; i+=get_local_size(0))\n"
                    "   {\n"
                    "      sum+= " +U::name() +"[i];\n"
                    "   };\n"
                    "   shared_memory_ptr[get_local_id(0)]=sum;\n"
                    "   for (unsigned int stride = get_local_size(0)/2; stride > 0; stride /= 2)\n"
                    "   {\n"
                    "      barrier(CLK_LOCAL_MEM_FENCE);\n"
                    "      if (get_local_id(0) < stride)\n"
                    "      shared_memory_ptr[get_local_id(0)] += shared_memory_ptr[get_local_id(0)+stride];\n"
                    "   }\n"
                    "   if(get_local_id(0)==0)\n"
                    "       "+U::local_value() + " = shared_memory_ptr[0];\n"
                    "   barrier(CLK_LOCAL_MEM_FENCE);\n"
                    "}\n";
        }
    };

    static const std::string value(){
        std::string res;
        typelist_utils::ForEach<T,generate_code>::execute(res);
        return res;
    }
};



template<class T>
struct make_code<MatVecToken<T> >
{
private:
    template<class U>
    struct fill_dot_prod
    {
        typedef typename U::LHS                                   LHS;
        typedef typename U::RHS                                   RHS;
        typedef typename result_of::expression_type<LHS>::Result    MatExpr;
        typedef typename MatExpr::ScalarType                        ScalarType;
        typedef typename MatExpr::Layout                            Layout;

        static const unsigned int Alignment = result_of::expression_type<LHS>::Result::Alignment;

    private:

        static const std::string evaluate(std::string dot_prod_name, viennacl::row_major)
        {
            std::string internal_size_2_expression = MatExpr::internal_size2_expression();
            VIENNACL_STATIC_ASSERT(Alignment==1 || Alignment==16,AlignmentNotSupported);
            if(Alignment==1)
                return  dot_prod_name + " +=  " + dot_product<LHS,RHS>::value("gid *" + internal_size_2_expression + " + col","col") + ";\n";
            else{
                return dot_prod_name + ".s0 +=  " + dot_product<LHS,RHS>::value("scaled_row *" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".s1 +=  " + dot_product<LHS,RHS>::value("(scaled_row+1)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".s2 +=  " + dot_product<LHS,RHS>::value("(scaled_row+2)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".s3 +=  " + dot_product<LHS,RHS>::value("(scaled_row+3)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".s4 +=  " + dot_product<LHS,RHS>::value("(scaled_row+4)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".s5 +=  " + dot_product<LHS,RHS>::value("(scaled_row+5)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".s6 +=  " + dot_product<LHS,RHS>::value("(scaled_row+6)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".s7 +=  " + dot_product<LHS,RHS>::value("(scaled_row+7)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".s8 +=  " + dot_product<LHS,RHS>::value("(scaled_row+8)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".s9 +=  " + dot_product<LHS,RHS>::value("(scaled_row+9)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".sa +=  " + dot_product<LHS,RHS>::value("(scaled_row+10)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".sb +=  " + dot_product<LHS,RHS>::value("(scaled_row+11)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".sc +=  " + dot_product<LHS,RHS>::value("(scaled_row+12)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".sd +=  " + dot_product<LHS,RHS>::value("(scaled_row+13)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".se +=  " + dot_product<LHS,RHS>::value("(scaled_row+14)*" + internal_size_2_expression + " + col","col") + ";\n"
                        + dot_prod_name + ".sf +=  " + dot_product<LHS,RHS>::value("(scaled_row+15)*" + internal_size_2_expression + " + col","col") + ";\n";
            }
        }

        static const std::string evaluate(std::string dot_prod_name, viennacl::column_major)
        {
            std::string internal_size_1_expression = MatExpr::internal_size1_expression();
            VIENNACL_STATIC_ASSERT(Alignment==1,AlignmentNotSupported);
            return   dot_prod_name + "+=  " + dot_product<LHS,RHS>::value("gid  + col * " +  internal_size_1_expression, "col") + ";\n";
        }
    public:
        static void execute(std::string & res)
        {
//            std::string dot_prod_name = "dp"+U::name();
//            res += print_type<ScalarType,Alignment>::value()+ " " + dot_prod_name + " = 0;\n";
//            res += "{";
//            res += "   unsigned int row_gid = get_global_id(0)/get_local_size(0);\n";
//            res += "   unsigned int col_gid = get_global_id(0)%get_local_size(0);\n";
//            res += "   unsigned int lid = get_local_id(0);";
//            res += "   for(unsigned int row = row_gid ; row < n_rows ; row+=get_num_groups(0)){";
//            res += "       ScalarType sum = 0;\n";
//            res += "       for(unsigned int col = col_gid ; col < n_cols ; col+=get_local_size(0)){\n";
//            res += "            sum+=mat[row*n_cols+col]*vec[col];\n";
//            res += "       }";
//            res += "       work[lid]=sum;";
//            res += "       for(unsigned int stride=get_local_size(0)/2 ; stride>0 ; stride>>=1){";
//            res += "           barrier(CLK_LOCAL_MEM_FENCE);\n";
//            res += "           if(lid < stride) work[lid]+=work[lid+stride];\n";
//            res += "       }\n";
//            res += "       if(lid==0) res[row]=work[0];";
//            res += "   }";
//            res += "}";
        }
    };

    typedef typename result_of::expression_type<typename T::Head>::Result    IntermediateType;
    static const unsigned int Alignment = IntermediateType::Alignment;

public:
    static const std::string value()
    {
        std::string res;
        if(Alignment!=1) res+= " unsigned int scaled_row = gid * " + to_string(Alignment) + ";\n";
        typelist_utils::ForEach<T, fill_dot_prod>::execute(res);
        return res;
    }

};


//Matrix-Matrix Product.

template<class T, class OP, class Assigned>
struct make_code<MatMatToken<T,OP,Assigned> >
{
private:

    static bool replace(std::string& str, const std::string& from, const std::string& to) {
        size_t start_pos = str.find(from);
        if(start_pos == std::string::npos)
            return false;
        str.replace(start_pos, from.length(), to);
        return true;
    }


    typedef typename tree_utils::remove_if<T, is_product_leaf>::Result                           SCALAR_EXPR;
    typedef typename tree_utils::extract_if<T,is_product_leaf>::Result::Head                     ARG;
    typedef typename ARG::LHS                                   LHS;
    typedef typename ARG::RHS                                   RHS;
    typedef typename result_of::expression_type<LHS>::Result    MatExpr;
    typedef typename MatExpr::ScalarType                        ScalarType;
    typedef typename MatExpr::Layout                            Layout;
    static const unsigned int Alignment = result_of::expression_type<LHS>::Result::Alignment;

public:
    static const std::string value(){
        VIENNACL_STATIC_ASSERT(Alignment==1,AlignmentNotSupported);
        static const std::size_t block_size = 16;
        static const std::size_t vector_size =  4;
        std::string res;
        res += "{\n";
        res+= "  size_t row_block_id = get_group_id(1);\n";    //refers to the row index in op(A), op(B)
        res += "  size_t col_block_id = get_group_id(0);\n";    //refers to the col index in op(A), op(B)
        res += "  size_t row_thread_id = get_local_id(1);\n";
        res += "  size_t col_thread_id = get_local_id(0);\n";
        res += "  __local float As[" + to_string(block_size * block_size) + "];\n";
        res += "  float cv[" + to_string(block_size) + "] = {";;
        for (std::size_t i=0; i<block_size-1; ++i)
            res += "0,";
        res += "0};\n" ;

        if (is_row_major<LHS>::value && is_transposed<LHS>::value)
        {
            res += "  size_t aBegin = (row_block_id * " + to_string(block_size) + " * " + LHS::col_inc_name() + " + " + LHS::col_start_name() + ") + " + LHS::row_start_name() + " * " + LHS::internal_size2_name()  + ";\n";
            res += "  size_t aStep = " + to_string(block_size) + " * " + LHS::internal_size2_name()  + " * " + LHS::row_inc_name() + ";\n";
            res += "  size_t aEnd = aBegin + " + LHS::internal_size2_name()  + " * " + LHS::row_inc_name() + " * " + LHS::size1_name() + ";\n";
        }
        else if (is_row_major<LHS>::value && !is_transposed<LHS>::value)
        {
            res += "  size_t aBegin = (row_block_id * " + to_string(block_size) + " * " + LHS::row_inc_name() + " + " + LHS::row_start_name() + ") * " + LHS::internal_size2_name()  + " + " + LHS::col_start_name() + ";\n";
            res += "  size_t aStep = " + to_string(block_size) + " * " + LHS::col_inc_name() + ";\n";
            res += "  size_t aEnd = aBegin + " + LHS::col_inc_name() + " * " + LHS::size2_name() + ";\n";
        }
        else if (!is_row_major<LHS>::value && is_transposed<LHS>::value)
        {
            res += "  size_t aBegin = (row_block_id * " + to_string(block_size) + " * " + LHS::col_inc_name() + " + " + LHS::col_start_name() + ") * " + LHS::internal_size1_name()  + " + " + LHS::row_start_name() + ";\n";
            res += "  size_t aStep = " + to_string(block_size) + " * " + LHS::row_inc_name() + ";\n";
            res += "  size_t aEnd = aBegin + " + LHS::row_inc_name() + " * " + LHS::size1_name() + ";\n";
        }
        else if (!is_row_major<LHS>::value && !is_transposed<LHS>::value)
        {
            res += "  size_t aBegin = (row_block_id * " + to_string(block_size) + " * " + LHS::row_inc_name() + " + " + LHS::row_start_name() + ") + " + LHS::col_start_name() + " * " + LHS::internal_size1_name()  + ";\n";
            res += "  size_t aStep = " + to_string(block_size) + " * " + LHS::internal_size1_name()  + " * " + LHS::col_inc_name() + ";\n";
            res += "  size_t aEnd = aBegin + " + LHS::internal_size1_name()  + " * " + LHS::col_inc_name() + " * " + LHS::size2_name() + ";\n";
        }


        if (is_row_major<RHS>::value && is_transposed<RHS>::value)
        {
            res += "  size_t bBegin = (col_block_id * " + to_string(block_size * vector_size) + " * " + RHS::row_inc_name() + " + " + RHS::row_start_name() + ") * " + RHS::internal_size2_name()  + " + " + RHS::col_start_name() + ";\n";
            res += "  size_t bStep = " + to_string(block_size) + " * " + RHS::col_inc_name() + ";\n";
        }
        else if (is_row_major<RHS>::value && !is_transposed<RHS>::value)
        {
            res += "  size_t bBegin = (col_block_id * " + to_string(block_size * vector_size) + " * " + RHS::col_inc_name() + " + " + RHS::col_start_name() + ") + " + RHS::row_start_name() + " * " + RHS::internal_size2_name()  + ";\n";
            res += "  size_t bStep = " + to_string(block_size) + " * " + RHS::row_inc_name() + " * " + RHS::internal_size2_name()  + ";\n";
        }
        else if (!is_row_major<RHS>::value && is_transposed<RHS>::value)
        {
            res += "  size_t bBegin = (col_block_id * " + to_string(block_size * vector_size) + " * " + RHS::row_inc_name() + " + " + RHS::row_start_name() + ") + " + RHS::col_start_name() + " * " + RHS::internal_size1_name()  + ";\n";
            res += "  size_t bStep = " + to_string(block_size) + " * " + RHS::col_inc_name() + " * " + RHS::internal_size1_name()  + ";\n";
        }
        else if (!is_row_major<RHS>::value && !is_transposed<RHS>::value)
        {
            res += "  size_t bBegin = (col_block_id * " + to_string(block_size * vector_size) + " * " + RHS::col_inc_name() + " + " + RHS::col_start_name() + ") * " + RHS::internal_size1_name()  + " + " + RHS::row_start_name() + ";\n";
            res += "  size_t bStep = " + to_string(block_size) + " * " + RHS::row_inc_name() + ";\n";
        }

        res += "  for(size_t a = aBegin, b = bBegin; a < aEnd; a += aStep, b += bStep) { \n";

        // copy blocks of op(A) to shared memory (op(A) is column-major in shared memory then)
        res += "    for(size_t i = 0; i < " + to_string(vector_size) + "; i++)  \n";
        if (is_row_major<LHS>::value && is_transposed<LHS>::value)
            res += "      As[ (i*" + to_string(vector_size) + " + row_thread_id) + " + to_string(block_size) + " * col_thread_id] = (" + make_expression_code<LHS>::value("a + " + LHS::col_inc_name() + " * (i * " + to_string(vector_size) + " + row_thread_id) + " + LHS::internal_size2_name()  + " * " + LHS::row_inc_name() + " * col_thread_id") + ");\n";
        else if (is_row_major<LHS>::value && !is_transposed<LHS>::value)
            res += "      As[ (i*" + to_string(vector_size) + " + row_thread_id) + " + to_string(block_size) + " * col_thread_id] = (" + make_expression_code<LHS>::value("a + " + LHS::internal_size2_name()  + " * " + LHS::row_inc_name() + " * (i * " + to_string(vector_size) + " + row_thread_id) + " + LHS::col_inc_name() + " * col_thread_id") + ");\n";
        else if (!is_row_major<LHS>::value && is_transposed<LHS>::value)
            res += "      As[ (i*" + to_string(vector_size) + " + row_thread_id) + " + to_string(block_size) + " * col_thread_id] = (" + make_expression_code<LHS>::value("a + " + LHS::internal_size1_name()  + " * " + LHS::col_inc_name() + " * (i * " + to_string(vector_size) + " + row_thread_id) + " + LHS::row_inc_name() + " * col_thread_id") + ");\n";
        else if (!is_row_major<LHS>::value && !is_transposed<LHS>::value)
            res += "      As[ (i*" + to_string(vector_size) + " + row_thread_id) + " + to_string(block_size) + " * col_thread_id] = (" + make_expression_code<LHS>::value("a + " + LHS::row_inc_name() + " * (i * " + to_string(vector_size) + " + row_thread_id) + " + LHS::internal_size1_name()  + " * " + LHS::col_inc_name() + " * col_thread_id") + ");\n";
        res += "\n";
        res += "    barrier(CLK_LOCAL_MEM_FENCE); \n";

        // initialize memory pointers
        res += "\n";
        res += "    __local  float *ap = As; \n";
        if (is_row_major<RHS>::value && is_transposed<RHS>::value)
            res += "    __global float *bp = " + RHS::name() + " + (b + (" + to_string(block_size) + " * row_thread_id + col_thread_id) * " + RHS::row_inc_name() + " * " + RHS::internal_size2_name()  + "); \n";
        else if (is_row_major<RHS>::value && !is_transposed<RHS>::value)
            res += "    __global float *bp = " + RHS::name() + " + (b + (" + to_string(block_size) + " * row_thread_id + col_thread_id) * " + RHS::col_inc_name() + "); \n";
        else if (!is_row_major<RHS>::value && is_transposed<RHS>::value)
            res += "    __global float *bp = " + RHS::name() + " + (b + (" + to_string(block_size) + " * row_thread_id + col_thread_id) * " + RHS::row_inc_name() + "); \n";
        else if (!is_row_major<RHS>::value && !is_transposed<RHS>::value)
            res += "    __global float *bp = " + RHS::name() + " + (b + (" + to_string(block_size) + " * row_thread_id + col_thread_id) * " + RHS::col_inc_name() + " * " + RHS::internal_size1_name()  + "); \n";
        res += "\n";

        std::string rhs_expr;

        if (is_row_major<RHS>::value && is_transposed<RHS>::value)
            rhs_expr = make_expression_code<RHS>::value("i");
        else if (is_row_major<RHS>::value && !is_transposed<RHS>::value)
            rhs_expr = make_expression_code<RHS>::value("i * " + RHS::internal_size2_name());
        else if (!is_row_major<RHS>::value && is_transposed<RHS>::value)
            rhs_expr = make_expression_code<RHS>::value("i * " + RHS::internal_size1_name());
        else if (!is_row_major<RHS>::value && !is_transposed<RHS>::value)
            rhs_expr = make_expression_code<RHS>::value("i");



        replace(rhs_expr,RHS::name(),"bp");

        // run computations
        res += "    for(size_t i = 0; i < " + to_string(block_size) + "; i++) { \n";
        res += "      float bv = " + rhs_expr + "; \n";
        res += "\n";
        res += "      for(size_t k = 0; k < " + to_string(block_size) + "; k++)  \n";
        res += "	    cv[k] += ap[k] * bv; \n";
        res += "\n";
        res += "      ap += " + to_string(block_size) + "; \n";
        res += "    } \n";
        res += "\n";
        res += "    barrier(CLK_LOCAL_MEM_FENCE); \n";
        res += "  } \n";

        // write to C
        if (is_row_major<Assigned>::value)
        {
            res += "  int c = " + Assigned::internal_size2_name()  + " * (" + Assigned::row_inc_name() + " * " + to_string(block_size) + " * row_block_id + " + Assigned::row_start_name() + ") + "  //block row index
                    + to_string(vector_size * block_size) + " * " + Assigned::col_inc_name() + " * col_block_id + " + Assigned::col_start_name() + " \n";  //block column index
            res += "          + " + Assigned::col_inc_name() + " * (" + to_string(block_size) + " * row_thread_id + col_thread_id); \n";
        }
        else
        {
            res += "  int c = " + Assigned::row_inc_name() + " * " + to_string(block_size) + " * row_block_id + " + Assigned::row_start_name() + " + ("   // block row index
                    + to_string(vector_size * block_size) + " * " + Assigned::col_inc_name() + " * col_block_id + " + Assigned::col_start_name() + ") * " + Assigned::internal_size1_name()  + " \n";   // block column index
            res += "          + " + Assigned::internal_size1_name()  +  " * " + Assigned::col_inc_name() + " * (" + to_string(block_size) + " * row_thread_id + col_thread_id); \n";
        }

        res += "  for(size_t i = 0; i < " + to_string(block_size) + "; i++) { \n";


        if (is_row_major<Assigned>::value)
        {
            if(is_null_type<SCALAR_EXPR>::value){
                res += "    " + Assigned::name() + "[c]" + OP::expression_string() + "cv[i]; \n";
                res += "      c += " + Assigned::internal_size2_name()  + " * " + Assigned::row_inc_name() + "; \n";
            }
            else{
                res += "    " + Assigned::name() + "[c]" + OP::expression_string() + make_expression_code<SCALAR_EXPR>::value ("") + "* cv[i]; \n";
                res += "      c += " + Assigned::internal_size2_name()  + " * " + Assigned::row_inc_name() + "; \n";
            }
        }
        else
        {
            if(is_null_type<SCALAR_EXPR>::value){
                res += "    " + Assigned::name() + "[c]" + OP::expression_string() + "cv[i]; \n";
                res += "      c += " + Assigned::row_inc_name() + "; \n";
            }
            else{
                res += "    " + Assigned::name() + "[c]" + OP::expression_string() + make_expression_code<SCALAR_EXPR>::value ("") + "* cv[i]; \n";
                res += "      c += " + Assigned::row_inc_name() + "; \n";
            }
        }

        res += "  } \n";
        res += "} \n";

        return res;
    }
};

}

}

#endif
