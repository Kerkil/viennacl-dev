#ifndef VIENNACL_DEVICE_SPECIFIC_TEMPLATES_VECTOR_AXPY_HPP
#define VIENNACL_DEVICE_SPECIFIC_TEMPLATES_VECTOR_AXPY_HPP

/* =========================================================================
   Copyright (c) 2010-2013, Institute for Microelectronics,
                            Institute for Analysis and Scientific Computing,
                            TU Wien.
   Portions of this software are copyright by UChicago Argonne, LLC.

                            -----------------
                  ViennaCL - The Vienna Computing Library
                            -----------------

   Project Head:    Karl Rupp                   rupp@iue.tuwien.ac.at

   (A list of authors and contributors can be found in the PDF manual)

   License:         MIT (X11), see file LICENSE in the base directory
============================================================================= */


/** @file viennacl/generator/vector_axpy.hpp
 *
 * Kernel template for the vector axpy-like operations
*/

#include <vector>

#include "viennacl/scheduler/forwards.h"

#include "viennacl/device_specific/mapped_objects.hpp"
#include "viennacl/device_specific/tree_parsing/read_write.hpp"
#include "viennacl/device_specific/tree_parsing/evaluate_expression.hpp"
#include "viennacl/device_specific/forwards.h"
#include "viennacl/device_specific/utils.hpp"

#include "viennacl/device_specific/templates/template_base.hpp"

#include "viennacl/tools/tools.hpp"

namespace viennacl
{

  namespace device_specific
  {

    class vector_axpy_template : public template_base
    {

    public:
      class parameters : public template_base::parameters
      {
      public:
        parameters(const char * scalartype, unsigned int simd_width,
                   unsigned int group_size, unsigned int num_groups,
                   unsigned int decomposition) : template_base::parameters(scalartype, simd_width, group_size, 1, 1), num_groups_(num_groups), decomposition_(decomposition){ }

        unsigned int num_groups() const { return num_groups_; }
        unsigned int decomposition() const { return decomposition_; }

      private:
        unsigned int num_groups_;
        unsigned int decomposition_;
      };

    private:
      void core(unsigned int /*kernel_id*/, utils::kernel_generation_stream& stream, statements_container const & statements, std::vector<mapping_type> const & mapping) const
      {
        stream << "for(unsigned int i = get_global_id(0) ; i < N ; i += get_global_size(0))" << std::endl;
        stream << "{" << std::endl;
        stream.inc_tab();

        //Registers already allocated
        std::set<std::string>  cache;

        //Fetch
        std::string rhs_suffix = "reg";
        std::string lhs_suffix = statements.order()==statements_container::INDEPENDENT?"tmp":rhs_suffix;

        for(statements_container::data_type::const_iterator it = statements.data().begin() ; it != statements.data().end() ; ++it)
        {
          tree_parsing::read_write(tree_parsing::read_write_traversal::FETCH, lhs_suffix, cache, *it, it->root(), index_tuple("i", "N"), stream,mapping[std::distance(statements.data().begin(),it)], tree_parsing::LHS_NODE_TYPE);
          tree_parsing::read_write(tree_parsing::read_write_traversal::FETCH, rhs_suffix, cache, *it, it->root(), index_tuple("i", "N"), stream,mapping[std::distance(statements.data().begin(),it)], tree_parsing::RHS_NODE_TYPE);
        }

        //Generates all the expression, in order
        unsigned int i = 0;
        for(statements_container::data_type::const_iterator it = statements.data().begin() ; it != statements.data().end() ; ++it)
          stream << tree_parsing::evaluate_expression(*it, it->root(), index_tuple("i", "N"), -1, mapping[i++], tree_parsing::PARENT_NODE_TYPE) << ";" << std::endl;

        //Write back
        for(statements_container::data_type::const_iterator it = statements.data().begin() ; it != statements.data().end() ; ++it)
        {
          tree_parsing::read_write(tree_parsing::read_write_traversal::WRITE_BACK, lhs_suffix, cache,*it, it->root(), index_tuple("i", "N"), stream,mapping[std::distance(statements.data().begin(),it)], tree_parsing::LHS_NODE_TYPE);
        }

        stream.dec_tab();
        stream << "}" << std::endl;
      }

      void add_kernel_arguments(statements_container const & statements, std::string & arguments_string) const
      {
        arguments_string += generate_value_kernel_argument("unsigned int", "N");
      }

      void configure_impl(unsigned int /*kernel_id*/, statements_container const & statements, viennacl::ocl::kernel & k, unsigned int & n_arg)  const
      {
        k.global_work_size(0,parameters_.local_size_0()*parameters_.num_groups());
        k.global_work_size(1,1);

        scheduler::statement_node const & root = statements.data().front().array()[statements.data().front().root()];
        cl_uint size;
        if(up_to_internal_size_)
          size = utils::call_on_vector(root.lhs, utils::internal_size_fun());
        else
          size = utils::call_on_vector(root.lhs, utils::size_fun());
        k.arg(n_arg++, size/parameters_.simd_width());
      }

    public:
      vector_axpy_template(vector_axpy_template::parameters const & parameters, binding_policy_t binding_policy = BIND_ALL_UNIQUE) : template_base(parameters, binding_policy), parameters_(parameters){ }

      void enqueue(std::string const & program_name, statements_container const & statements, bool up_to_internal_size = false)
      {
        up_to_internal_size_ = up_to_internal_size;
        template_base::enqueue(program_name, statements);
      }

    private:
      vector_axpy_template::parameters const & parameters_;
      bool up_to_internal_size_;
    };

  }

}

#endif
