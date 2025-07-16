#pragma once

#include <vector>
#include <memory>
#include <functional>
#include "DataParser.h"

namespace ObjectTraversalUtils {
    // Find and remove object from a vector, returns true if found and removed
    bool FindAndRemoveObject(std::vector<std::shared_ptr<DataParser::DataObject>>& objects, 
                            std::shared_ptr<DataParser::DataObject> targetObj);

    // Find object by UID in a vector, returns the object if found
    std::shared_ptr<DataParser::DataObject> FindObjectByUID(const std::vector<std::shared_ptr<DataParser::DataObject>>& objects, 
                                                           uint32_t uid);

    // Find and replace object in a vector, returns true if found and replaced
    bool FindAndReplaceObject(std::vector<std::shared_ptr<DataParser::DataObject>>& objects,
                             std::shared_ptr<DataParser::DataObject> targetObj,
                             std::shared_ptr<DataParser::DataObject> replacementObj);

    // Count total objects including nested ones
    int CountObjects(const std::vector<std::shared_ptr<DataParser::DataObject>>& objects);

    // Find the parent vector containing the target object, returns nullptr if not found
    std::vector<std::shared_ptr<DataParser::DataObject>>* FindParentVector(
        std::vector<std::shared_ptr<DataParser::DataObject>>& objects,
        std::shared_ptr<DataParser::DataObject> targetObj);

    // Insert an object after a target object in the hierarchy
    bool InsertAfterTarget(std::vector<std::shared_ptr<DataParser::DataObject>>& objects,
                          std::shared_ptr<DataParser::DataObject> toInsert,
                          std::shared_ptr<DataParser::DataObject> target);

    // Move an object up in its parent vector
    bool MoveObjectUp(std::vector<std::shared_ptr<DataParser::DataObject>>& objects,
                     std::shared_ptr<DataParser::DataObject> targetObj);

    // Move an object down in its parent vector
    bool MoveObjectDown(std::vector<std::shared_ptr<DataParser::DataObject>>& objects,
                       std::shared_ptr<DataParser::DataObject> targetObj);

    // Template function to traverse all objects recursively and apply a function
    // Returns true if the function returns true for any object (early termination)
    template<typename Func>
    bool ForEachObjectRecursive(std::vector<std::shared_ptr<DataParser::DataObject>>& objects, Func&& func) {
        for (auto& obj : objects) {
            if (func(obj)) {
                return true; // Early termination
            }
            
            // Check nested objects
            if (obj->isNested()) {
                auto& nested = obj->getNestedObjects();
                if (ForEachObjectRecursive(nested, std::forward<Func>(func))) {
                    return true; // Early termination
                }
            }
        }
        return false;
    }

    // Template function to traverse all objects recursively and apply a function (const version)
    // Returns true if the function returns true for any object (early termination)
    template<typename Func>
    bool ForEachObjectRecursive(const std::vector<std::shared_ptr<DataParser::DataObject>>& objects, Func&& func) {
        for (const auto& obj : objects) {
            if (func(obj)) {
                return true; // Early termination
            }
            
            // Check nested objects
            if (obj->isNested()) {
                const auto& nested = obj->getNestedObjects();
                if (ForEachObjectRecursive(nested, std::forward<Func>(func))) {
                    return true; // Early termination
                }
            }
        }
        return false;
    }
} 