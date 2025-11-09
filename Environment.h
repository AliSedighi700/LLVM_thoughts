#ifndef Environment_H
#define Environment_H

#include <map>
#include <memory>
#include <string>

#include "./Logger.h"
#include "llvm/IR/Value.h"

class Environment : public std::enable_shared_from_this<Environment>
{
public:

    Environment(std::map<std::string, llvm::value*> record,
               std::shared_ptr<Environment> parent)
    : record_(record), parent_(parent)
    {}

    //create a variable with given name ans the value;
    llvm::value* define(const std::string name, llvm::value* value)
    {
        record_[name] = value;
        return value;
    }

    //to access the value. return the value of the
    //defined variable or thrown if it is not defined
    //after it is searched in the whole environment chain
    //using resolve to traverse in them. so resolve
    //should send an environment in which the actual
    //variable is found.
    llvm::value* lookup(const std::string name)
    {
        return resolve(name) -> record_[name];
    }

Privae:

    std::shared_ptr<Environment> resolve(const std::string& name)
    {
        if(record_.count(name) != 0){return shared_from_this();}

        if(parent_ == nullptr)
        {
            DIE << "Variable " << name << " is not defined" << '\n';
        }

        return parent_ -> resolve(name);
    }

    // The main component of the enivironment.
    // The actual storage
    std::map<std::string, llvm::value*> record_{};

    //multile environments can share the same
    //parent environment
    std::shared_ptr<Environment> parent_{};


};

#endif Environment_H
