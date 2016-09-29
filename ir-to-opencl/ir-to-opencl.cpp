// Copyright Hugh Perkins 2016

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// input: IR from cuda compilation

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <stdexcept>
#include <sstream>

using namespace llvm;
using namespace std;

static llvm::LLVMContext TheContext;
static llvm::IRBuilder<> Builder(TheContext);
static std::unique_ptr<llvm::Module> TheModule;
static std::map<std::string, Value *> NamedValues;
static std::map<string, bool> iskernel_by_name;
static std::set<string> ignoredFunctionNames;
static std::map<string, string> knownFunctionsMap; // from cuda to opencl, eg tid.x => get_global_id

map<Value *, string> nameByValue;
static int nextNameIdx;

static bool debug;
static bool single_precision = true;

std::string dumpType(Type *type);

std::string dumpFunctionType(FunctionType *fn) {
    cout << "function" << endl;
    // cout << "name " << string(fn->getName()) << endl;
    std::string params_str = "";
    int i = 0;
    for(auto it=fn->param_begin(); it != fn->param_end(); it++) {
        Type * paramType = *it;
        if(i > 0) {
            params_str += ", ";
        }
        params_str += dumpType(paramType);
        i++;
    }
    cout << "params_str " << params_str << endl;
    return params_str;
}

std::string dumpPointerType(PointerType *ptr) {
    string gencode = "";
    Type *elementType = ptr->getPointerElementType();
    string elementTypeString = dumpType(elementType);
    int addressspace = ptr->getAddressSpace();
    if(addressspace == 1) {
        gencode += "global ";
    }
    gencode += elementTypeString + "*";
    return gencode;
}

std::string dumpType(Type *type) {
    Type::TypeID typeID = type->getTypeID();
    switch(typeID) {
        case Type::VoidTyID:
            return "void";
        case Type::FloatTyID:
            return "float";
        case Type::DoubleTyID:
            if(single_precision) {
                return "float";
            } else {
                return "double";
            }
        case Type::FunctionTyID:
            return dumpFunctionType((FunctionType *)type);
        case Type::PointerTyID:
            return dumpPointerType((PointerType *)type);
        case Type::IntegerTyID:
            switch(type->getPrimitiveSizeInBits()) {
                case 32:
                    return "int";
                case 64:
                    return "long";
                case 1:
                    return "bool";
                default:
                    cout << "integer size " << type->getPrimitiveSizeInBits() << endl;
                    throw runtime_error("unrecognized size");
            }
        default:
            cout << "type id " << typeID << endl;
            throw runtime_error("unrecognized type");
    }
}

std::string dumpValue(Value *value) {
    std::string gencode = "";
    unsigned int valueTy = value->getValueID();
    switch(valueTy) {
        case AShrOperator::ConstantIntVal:
            cout << "constant " << ((ConstantInt *)value)->getSExtValue() << endl;
    }
    string name = nameByValue[value];
    gencode += name;
    return gencode;
}

int getIntConstant(Value *value) {
    unsigned int valueTy = value->getValueID();
    if(valueTy != AShrOperator::ConstantIntVal) {
        throw runtime_error("not a constant int");
    }
    return ((ConstantInt *)value)->getSExtValue();
}

double getFloatConstant(Value *value) {
    unsigned int valueTy = value->getValueID();
    if(valueTy != AShrOperator::ConstantFPVal) {
        throw runtime_error("not a constant float");
    }
    return ((ConstantFP *)value)->getValueAPF().convertToFloat();
}

string dumpConstant(Constant *constant) {
    unsigned int valueTy = constant->getValueID();
    ostringstream oss;
    if(valueTy == AShrOperator::ConstantIntVal) {
        oss << "int:" << ((ConstantInt *)constant)->getSExtValue();
    } else if(GlobalValue::classof(constant)) {
        GlobalValue *global = (GlobalValue *)constant;
        PointerType *pointerType = global->getType();
        Type *elementType = pointerType->getPointerElementType();
        cout << "element type " << elementType << endl;
        cout << dumpType(elementType) << endl;
        cout << "constant has name " << constant->hasName() << " " << string(constant->getName()) << endl;
    } else {
        cout << "valueTy " << valueTy << endl;
        cout << GlobalValue::classof(constant) << endl;
        oss << "unknown";
    }
    return oss.str();
}

string dumpOperand(Value *value) {
    unsigned int valueTy = value->getValueID();
    if(valueTy == AShrOperator::ConstantIntVal) {
        int intvalue = getIntConstant(value);
        ostringstream oss;
        oss << intvalue;
        return oss.str();
    }
    if(valueTy == AShrOperator::ConstantFPVal) {
        double floatvalue = getFloatConstant(value);
        ostringstream oss;
        oss << floatvalue;
        return oss.str();
    }
    string name = nameByValue[value];
    return name;
}

void storeValueName(Value *value) {
    if(value->hasName()) {
        nameByValue[value] = string(value->getName());
    } else {
        int idx = nextNameIdx;
        nextNameIdx++;
        ostringstream oss;
        oss << "v" << idx;
        string name = oss.str();
        nameByValue[value] = name;
    }
}

std::string dumpReturn(ReturnInst *retInst) {
    std::string gencode = "";
    Value *retValue = retInst->getReturnValue();
    if(retValue != 0) {
        gencode += "return " + dumpOperand(retValue) + ";\n";
    }
    return gencode;
}

// std::string dumpAlloca(Instruction *alloca) {
//     std::string gencode = "";
//     std::string typestring = dumpType(alloca->getType()->getPointerElementType());
//     // cout << "alloca typestring " << typestring << endl;
//     // int count = getIntConstant(alloca->getOperand(0));
//     // cout << "count " << count << endl;
//     return gencode;
// }

void updateAddressSpace(Value *value, int newSpace) {
    Type *elementType = value->getType()->getPointerElementType();
    Type *newType = PointerType::get(elementType, newSpace);
    value->mutateType(newType);
}

string dumpLoad(LoadInst *instr) {
    string gencode = "";
    string typestr = dumpType(instr->getType());
    gencode += typestr + " " + dumpOperand(instr) + " = " + dumpOperand(instr->getOperand(0)) + "[0];\n";
    return gencode;
}

string dumpStore(StoreInst *instr) {
    string gencode = "";
    gencode += dumpOperand(instr->getOperand(1)) + "[0] = " + dumpOperand(instr->getOperand(0)) + ";\n";
    return gencode;
}

string dumpGetElementPtr(GetElementPtrInst *instr) {
    string gencode = "";
    PointerType *inType = (PointerType *)instr->getOperand(0)->getType();
    int addressspace = inType->getAddressSpace();
    if(addressspace != 0) {
        updateAddressSpace(instr, addressspace);
    }
    string typestr = dumpType(instr->getType());
    string offset = dumpOperand(instr->getOperand(1));
    gencode += typestr + " " + dumpOperand(instr) + " = " + dumpOperand(instr->getOperand(0)) + "[" + offset + "];\n";
    return gencode;
}

std::string dumpBinaryOperator(BinaryOperator *instr, std::string opstring) {
    string gencode = "";
    string typestr = dumpType(instr->getType());
    gencode += typestr + " " + dumpOperand(instr) + " = ";
    Value *op1 = instr->getOperand(0);
    gencode += dumpValue(op1) + " ";
    gencode += opstring + " ";
    Value *op2 = instr->getOperand(1);
    gencode += dumpOperand(op2) + ";\n";
    return gencode;
}

std::string dumpBitcast(BitCastInst *instr) {
    string gencode = "";
    Value *op0 = instr->getOperand(0);
    Type *op0type = instr->getOperand(0)->getType();
    if(op0type->getTypeID() == Type::PointerTyID) {
        PointerType *inType = (PointerType *)op0type;
        int addressspace = inType->getAddressSpace();
        if(addressspace != 0) {
            updateAddressSpace(instr, addressspace);
        }
    }
    gencode += dumpType(instr->getType());
    gencode += dumpOperand(instr) + " = (" + dumpType(instr->getType()) + ")" + dumpOperand(op0) + ";\n";
    return gencode;
}

std::string dumpCall(CallInst *instr) {
    string gencode = "";
    string typestr = dumpType(instr->getType());
    if(typestr != "void") {
        gencode += typestr + " " + dumpOperand(instr) + " = ";
    }

    string functionName = string(instr->getCalledValue()->getName());
    if(functionName == "llvm.ptx.read.tid.x") {
        return gencode + "get_global_id(0);\n";
    }
    if(functionName == "llvm.ptx.read.tid.y") {
        return gencode + "get_global_id(1);\n";
    }
    if(functionName == "llvm.ptx.read.tid.z") {
        return gencode + "get_global_id(2);\n";
    }
    if(functionName == "llvm.ptx.read.ctaid.x") {
        return gencode + "get_group_id(0);\n";
    }
    if(functionName == "llvm.ptx.read.ctaid.y") {
        return gencode + "get_group_id(1);\n";
    }
    if(functionName == "llvm.ptx.read.ctaid.z") {
        return gencode + "get_group_id(2);\n";
    }
    if(functionName == "llvm.ptx.read.nctaid.x") {
        return gencode + "get_num_groups(0);\n";
    }
    if(functionName == "llvm.ptx.read.nctaid.y") {
        return gencode + "get_num_groups(1);\n";
    }
    if(functionName == "llvm.ptx.read.nctaid.z") {
        return gencode + "get_num_groups(2);\n";
    }
    if(functionName == "llvm.ptx.read.ntid.x") {
        return gencode + "get_local_size(0);\n";
    }
    if(functionName == "llvm.ptx.read.ntid.y") {
        return gencode + "get_local_size(1);\n";
    }
    if(functionName == "llvm.ptx.read.ntid.z") {
        return gencode + "get_local_size(2);\n";
    }
    if(functionName == "llvm.cuda.syncthreads") {
        return gencode + "barrier(CLK_GLOBAL_MEM_FENCE);\n";
    }
    if(knownFunctionsMap.find(functionName) != knownFunctionsMap.end()) {
        // cout << "replace " << functionName << " with " << knownFunctionsMap[functionName] << endl;
        functionName = knownFunctionsMap[functionName];
    }
    gencode += functionName + "(";
    int i = 0;
    for(auto it=instr->arg_begin(); it != instr->arg_end(); it++) {
        Value *op = &*it->get();
        if(i > 0) {
            gencode += ", ";
        }
        gencode += dumpValue(op);
        i++;
    }
    gencode += ");\n";
    return gencode;
}

std::string dumpFPExt(CastInst *instr) {
    string gencode = "";
    string typestr = dumpType(instr->getType());
    gencode += typestr + " " + dumpOperand(instr) + " = ";
    gencode += dumpValue(instr->getOperand(0)) + ";\n";
    return gencode;
}

std::string dumpZExt(CastInst *instr) {
    string gencode = "";
    string typestr = dumpType(instr->getType());
    gencode += typestr + " " + dumpOperand(instr) + " = ";
    gencode += dumpValue(instr->getOperand(0)) + ";\n";
    return gencode;
}

std::string dumpSExt(CastInst *instr) {
    string gencode = "";
    string typestr = dumpType(instr->getType());
    gencode += typestr + " " + dumpOperand(instr) + " = ";
    gencode += dumpValue(instr->getOperand(0)) + ";\n";
    return gencode;
}

std::string dumpFPTrunc(CastInst *instr) {
    // since this is float point trunc, lets just assume we're going from double to float
    // fix any exceptiosn to this rule later
    string gencode = "";
    string typestr = dumpType(instr->getType());
    gencode += typestr + " " + dumpOperand(instr) + " = ";
    gencode += "(float)" + dumpValue(instr->getOperand(0)) + ";\n";
    return gencode;
}

std::string dumpIcmp(ICmpInst *instr) {
    string gencode = "";
    string typestr = dumpType(instr->getType());
    gencode += typestr + " " + dumpOperand(instr) + " = ";
    CmpInst::Predicate predicate = instr->getSignedPredicate();  // note: we should detect signedness...
    string predicate_string = "";
    switch(predicate) {
        case CmpInst::ICMP_SLT:
            predicate_string = "<";
            break;
        case CmpInst::ICMP_SGT:
            predicate_string = ">";
            break;
        default:
            cout << "predicate " << predicate << endl;
            throw runtime_error("predicate not supported");
    }
    gencode += dumpOperand(instr->getOperand(0));
    gencode += " " + predicate_string + " ";
    gencode += dumpOperand(instr->getOperand(1)) + ";\n";
    return gencode;
}

std::string dumpBasicBlock(BasicBlock *basicBlock) {
    std::string gencode = "";
    for(BasicBlock::iterator it=basicBlock->begin(), e=basicBlock->end(); it != e; it++) {
        Instruction *instruction = &*it;
        auto opcode = instruction->getOpcode();
        storeValueName(instruction);
        string resultName = nameByValue[instruction];
        string resultType = dumpType(instruction->getType());

        string instructioncode = "";
        if(debug) {
            cout << resultType << " " << resultName << " =";
            cout << " " << string(instruction->getOpcodeName());
            for(auto it=instruction->op_begin(); it != instruction->op_end(); it++) {
                Value *op = &*it->get();
                cout << " " << dumpOperand(op);
            }
            cout << endl;
        }
        if(opcode == Instruction::FAdd) {
            instructioncode = dumpBinaryOperator((BinaryOperator*)instruction, "+");
        } else if(opcode == Instruction::FSub) {
            instructioncode = dumpBinaryOperator((BinaryOperator*)instruction, "-");
        } else if(opcode == Instruction::FDiv) {
            instructioncode = dumpBinaryOperator((BinaryOperator*)instruction, "/");
        } else if(opcode == Instruction::FMul) {
            instructioncode = dumpBinaryOperator((BinaryOperator*)instruction, "*");
        } else if(opcode == Instruction::Add) {
            instructioncode = dumpBinaryOperator((BinaryOperator*)instruction, "+");
        } else if(opcode == Instruction::Mul) {
            instructioncode = dumpBinaryOperator((BinaryOperator*)instruction, "*");
        } else if(opcode == Instruction::SDiv) {
            instructioncode = dumpBinaryOperator((BinaryOperator*)instruction, "/");
        } else if(opcode == Instruction::And) {
            instructioncode = dumpBinaryOperator((BinaryOperator*)instruction, "&&");
        } else if(opcode == Instruction::Or) {
            instructioncode = dumpBinaryOperator((BinaryOperator*)instruction, "||");
        // } else if(opcode == Instruction::Alloca) {
        //     gencode += dumpAlloca(instruction);
        } else if(opcode == Instruction::Store) {
            instructioncode = dumpStore((StoreInst*)instruction);
        } else if(opcode == Instruction::Call) {
            instructioncode = dumpCall((CallInst *)instruction);
        } else if(opcode == Instruction::Load) {
            instructioncode = dumpLoad((LoadInst*)instruction);
        } else if(opcode == Instruction::ICmp) {
            instructioncode = dumpIcmp((ICmpInst*)instruction);
        // } else if(opcode == Instruction::Br) {
        } else if(opcode == Instruction::SExt) {
            instructioncode = dumpSExt((CastInst*)instruction);
        } else if(opcode == Instruction::ZExt) {
            instructioncode = dumpZExt((CastInst*)instruction);
        } else if(opcode == Instruction::FPExt) {
            instructioncode = dumpFPExt((CastInst *)instruction);
        } else if(opcode == Instruction::FPTrunc) {
            instructioncode = dumpFPTrunc((CastInst *)instruction);
        } else if(opcode == Instruction::BitCast) {
            instructioncode = dumpBitcast((BitCastInst *)instruction);
        } else if(opcode == Instruction::GetElementPtr) {
            instructioncode = dumpGetElementPtr((GetElementPtrInst *)instruction);
        } else if(opcode == Instruction::Ret) {
            instructioncode = dumpReturn((ReturnInst *)instruction);
        } else {
            cout << "opcode string " << instruction->getOpcodeName() << endl;
            throw runtime_error("unknown opcode");
        }
        if(debug) {
            cout << instructioncode << endl;
        }
        gencode += instructioncode;
    }
    return gencode;
}

void myDump(Function *F) {
    Type *retType = F->getReturnType();
    std::string retTypeString = dumpType(retType);
    string fname = F->getName();
    string gencode = "";
    if(iskernel_by_name[fname]) {
        gencode += "kernel ";
    }
    gencode += dumpType(retType) + " " + fname + "(";
    int i = 0;
    for(auto it=F->arg_begin(); it != F->arg_end(); it++) {
        Argument *arg = &*it;
        storeValueName(arg);
        Type *argType = arg->getType();
        if(argType->getTypeID() == Type::PointerTyID) {
            Type *elementType = argType->getPointerElementType();
            Type *newtype = PointerType::get(elementType, 1);
            arg->mutateType(newtype);
        }
        string argname = dumpType(arg->getType()) + " " + string(arg->getName());
        if(i > 0) {
            gencode += ", ";
        }
        gencode += argname;
        i++;
    }
    gencode += ") {\n";
    for(auto it=F->begin(); it != F->end(); it++) {
        BasicBlock *basicBlock = &*it;
        gencode += dumpBasicBlock(basicBlock);
    }
    gencode += "}\n";
    cout << gencode << endl;
}

void dumpModule(Module *M) {
    for(auto it=M->named_metadata_begin(); it != M->named_metadata_end(); it++) {
        NamedMDNode *namedMDNode = &*it;
        for(auto it2=namedMDNode->op_begin(); it2 != namedMDNode->op_end(); it2++) {
            MDNode *mdNode = *it2;
            bool isKernel = false;
            string kernelName = "";
            for(auto it3=mdNode->op_begin(); it3 != mdNode->op_end(); it3++) {
                const MDOperand *op = it3;
                Metadata *metadata = op->get();
                if(metadata != 0) {
                    if(MDString::classof(metadata)) {
                        string meta_value = string(((MDString *)metadata)->getString());
                        if(meta_value == "kernel") {
                            isKernel = true;
                        }
                    } else if(ConstantAsMetadata::classof(metadata)) {
                        Constant *constant = ((ConstantAsMetadata *)metadata)->getValue();
                        if(GlobalValue::classof(constant)) {
                            GlobalValue *global = (GlobalValue *)constant;
                            if(global->getType()->getPointerElementType()->getTypeID() == Type::FunctionTyID) {
                                string functionName = string(constant->getName());
                                kernelName = functionName;
                            }
                        }
                    }
                }
            }
            if(isKernel) {
                iskernel_by_name[kernelName] = true;
            }
        }
    }

    for(auto it = M->begin(); it != M->end(); it++) {
        nameByValue.clear();
        nextNameIdx = 0;
        string name = it->getName();
        if(ignoredFunctionNames.find(name) == ignoredFunctionNames.end() &&
                knownFunctionsMap.find(name) == knownFunctionsMap.end()) {
            Function *F = &*it;
            myDump(F);
        }
    }
}

int main(int argc, char *argv[]) {
    SMDiagnostic Err;
    if(argc < 2) {
        cout << "Usage: " << argv[0] << " [--debug] target.ll" << endl;
        return 1;
    }
    string target = argv[argc - 1];
    debug = false;
    if(argc == 3) {
        if(string(argv[1]) != "--debug") {
            cout << "Usage: " << argv[0] << " [--debug] target.ll" << endl;
            return 1;
        } else {
            debug = true;
        }
    }
    TheModule = parseIRFile(target, Err, TheContext);
    if(!TheModule) {
        Err.print(argv[0], errs());
        return 1;
    }
    ignoredFunctionNames.insert("llvm.ptx.read.tid.x");
    ignoredFunctionNames.insert("llvm.ptx.read.tid.y");
    ignoredFunctionNames.insert("llvm.ptx.read.tid.z");
    ignoredFunctionNames.insert("llvm.cuda.syncthreads");
    ignoredFunctionNames.insert("_ZL21__nvvm_reflect_anchorv");
    ignoredFunctionNames.insert("__nvvm_reflect");
    ignoredFunctionNames.insert("llvm.ptx.read.ctaid.x");
    ignoredFunctionNames.insert("llvm.ptx.read.ctaid.y");
    ignoredFunctionNames.insert("llvm.ptx.read.ctaid.z");
    ignoredFunctionNames.insert("llvm.ptx.read.nctaid.x");
    ignoredFunctionNames.insert("llvm.ptx.read.nctaid.y");
    ignoredFunctionNames.insert("llvm.ptx.read.nctaid.z");
    ignoredFunctionNames.insert("llvm.ptx.read.ntid.x");
    ignoredFunctionNames.insert("llvm.ptx.read.ntid.y");
    ignoredFunctionNames.insert("llvm.ptx.read.ntid.z");

    knownFunctionsMap["llvm.nvvm.sqrt.rn.d"] = "sqrt";
    knownFunctionsMap["_Z16our_pretend_tanhf"] = "tanh";
    knownFunctionsMap["_Z15our_pretend_logf"] = "log";
    knownFunctionsMap["_Z15our_pretend_expf"] = "exp";

    dumpModule(TheModule.get());
    return 0;
}

