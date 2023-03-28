#ifndef SYC_IR_FUNCTION_H_
#define SYC_IR_FUNCTION_H_

#include "common.h"

namespace syc {
namespace ir {

struct Function {
  std::string name;
  TypePtr return_type;

  std::vector<OperandID> parameter_ids;

  std::list<BasicBlockID> basic_block_list;

  std::vector<InstructionID> caller_ids;

  void add_basic_block(BasicBlockID basic_block_id);

  void remove_basic_block(BasicBlockID basic_block_id);

  void insert_basic_block_after(
    BasicBlockID basic_block_id,
    BasicBlockID insert_after_id
  );

  void insert_basic_block_before(
    BasicBlockID basic_block_id,
    BasicBlockID insert_before_id
  );

  std::string to_string(Context& context);
};

}  // namespace ir
}  // namespace syc

#endif